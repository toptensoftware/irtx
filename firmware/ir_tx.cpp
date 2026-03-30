#include <Arduino.h>
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "config.h"
#include "gpio.h"
#include "led.h"
#include "ir_tx.h"
#include "ir_protocol.h"
#include "ir_encoder.h"

// ---- RMT Globals ----
static rmt_channel_handle_t txChannel   = NULL;
static rmt_encoder_handle_t copyEncoder = NULL;

// ---- Symbols buffer ----
// Free immediately after rmt_transmit() returns (copy encoder copies to RMT memory).
static rmt_symbol_word_t s_rmtSymbols[MAX_TIMING_VALUES];
static int               s_rmtSymbolCount = 0;

// ---- In-flight state ----
// Tracks the RMT transmission and the inter-packet gap that follows it.
static bool     s_rmtBusy     = false;
static uint32_t s_currentGap  = 0;
static bool     s_gapWaiting  = false;
static int64_t  s_gapExpiresAt = 0;

// Protocol+code currently occupying the in-flight slot (only valid when set via handleIrCode).
static bool     s_inflightCodeValid = false;
static uint32_t s_inflightProtocol  = 0;
static uint64_t s_inflightCode      = 0;

// ---- Pending slot ----
// Holds the next transmission while the in-flight slot is busy.
static uint16_t s_pendingTimings[MAX_TIMING_VALUES + 1];
static int      s_pendingCount = 0;
static uint32_t s_pendingGap  = 0;
static bool     s_hasPending  = false;

// Protocol+code queued in the pending slot (only valid when set via handleIrCode).
static bool     s_pendingCodeValid = false;
static uint32_t s_pendingProtocol  = 0;
static uint64_t s_pendingCode      = 0;

// ---- Build RMT Symbols ----
static void buildRmtSymbols(uint16_t* timings, int count)
{
    struct Phase { uint8_t level; uint16_t duration; };
    static Phase phases[MAX_TIMING_VALUES * 2];
    int phaseCount = 0;

    for (int i = 0; i < count; i++)
    {
        uint8_t level = (i % 2 == 0) ? 1 : 0;
        uint16_t remaining = timings[i];
        while (remaining > 0 && phaseCount < MAX_TIMING_VALUES * 2)
        {
            uint16_t chunk = (remaining > 32767) ? 32767 : remaining;
            phases[phaseCount++] = {level, chunk};
            remaining -= chunk;
        }
    }

    s_rmtSymbolCount = 0;
    for (int i = 0; i < phaseCount; i += 2)
    {
        s_rmtSymbols[s_rmtSymbolCount].level0    = phases[i].level;
        s_rmtSymbols[s_rmtSymbolCount].duration0 = phases[i].duration;
        if (i + 1 < phaseCount)
        {
            s_rmtSymbols[s_rmtSymbolCount].level1    = phases[i+1].level;
            s_rmtSymbols[s_rmtSymbolCount].duration1 = phases[i+1].duration;
        }
        else
        {
            s_rmtSymbols[s_rmtSymbolCount].level1    = 0;
            s_rmtSymbols[s_rmtSymbolCount].duration1 = 0;
        }
        s_rmtSymbolCount++;
    }
}

// ---- Start an RMT transmission (non-blocking) ----
// s_rmtSymbols must already be built. Buffer is free after this returns.
static void startTransmit(uint32_t gap)
{
    rmt_transmit_config_t txCfg = {};
    txCfg.loop_count = 0;
    esp_err_t err = rmt_transmit(txChannel, copyEncoder,
                                 s_rmtSymbols,
                                 s_rmtSymbolCount * sizeof(rmt_symbol_word_t),
                                 &txCfg);
    if (err != ESP_OK)
    {
        LOG("IR: transmit error: %s\n", esp_err_to_name(err));
        return;
    }
    // s_rmtSymbols is now free — copy encoder has transferred to RMT memory.
    setLed(LED_PRIORITY_ACTIVITY, 0x004000);    // Bright green
    s_rmtBusy    = true;
    s_currentGap = gap;
}

// ---- RMT Setup ----
void setupIrTx()
{
    if (IR_TX_PIN < 0) {
        LOG("IR TX disabled (no pin assigned)\n");
        return;
    }

    rmt_tx_channel_config_t txConfig = {};
    txConfig.gpio_num = (gpio_num_t)IR_TX_PIN;
    txConfig.clk_src = RMT_CLK_SRC_DEFAULT;
    txConfig.resolution_hz = 1000000; // 1MHz = 1us per tick
    txConfig.mem_block_symbols = 48;
    txConfig.trans_queue_depth = 1;
    ESP_ERROR_CHECK(rmt_new_tx_channel(&txConfig, &txChannel));

    rmt_carrier_config_t carrierConfig = {};
    carrierConfig.frequency_hz = CARRIER_FREQ;
    carrierConfig.duty_cycle = 0.33;
    ESP_ERROR_CHECK(rmt_apply_carrier(txChannel, &carrierConfig));
    ESP_ERROR_CHECK(rmt_enable(txChannel));

    rmt_copy_encoder_config_t encConfig = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&encConfig, &copyEncoder));

    LOG("RMT TX initialized on GPIO %d, Carrier: %d Hz\n", IR_TX_PIN, CARRIER_FREQ);
}

// ---- Busy check ----
bool isIrTxBusy()
{
    return s_rmtBusy || s_gapWaiting || s_hasPending;
}

// ---- In-flight code query ----
// Returns true and fills protocol/code if a known IR code is currently in-flight (RMT or gap).
// Returns false if IR TX is idle or the slot was filled by a raw timing packet.
bool getInflightIrCode(uint32_t* protocol, uint64_t* code)
{
    if (!s_inflightCodeValid) return false;
    *protocol = s_inflightProtocol;
    *code     = s_inflightCode;
    return true;
}

// ---- Poll ----
void pollIrTx()
{
    if (txChannel == NULL) return;

    // Check if RMT hardware has finished
    if (s_rmtBusy)
    {
        if (rmt_tx_wait_all_done(txChannel, 0) == ESP_OK)
        {
            s_gapExpiresAt = esp_timer_get_time() + s_currentGap;
            s_gapWaiting   = true;
            s_rmtBusy      = false;
            setLed(LED_PRIORITY_ACTIVITY, 0xFFFFFFFF);
        }
    }

    // Check if inter-packet gap has expired
    if (s_gapWaiting && esp_timer_get_time() >= s_gapExpiresAt)
        s_gapWaiting = false;

    // Start pending transmission once both RMT and gap are clear
    if (!s_rmtBusy && !s_gapWaiting && s_hasPending)
    {
        s_hasPending = false;
        // Promote pending code tracking to in-flight
        s_inflightCodeValid = s_pendingCodeValid;
        s_inflightProtocol  = s_pendingProtocol;
        s_inflightCode      = s_pendingCode;
        s_pendingCodeValid  = false;
        buildRmtSymbols(s_pendingTimings, s_pendingCount);
        if (s_rmtSymbolCount > 0)
            startTransmit(s_pendingGap);
    }
}

// ---- Common Transmit Helper ----
static void transmitTimingsNoLog(uint16_t* timings, int count, uint32_t gap)
{
    if (txChannel == NULL) return;

    // Ensure even count
    if (count % 2 != 0)
    {
        timings[count] = 0;
        count++;
    }

    if (!s_rmtBusy && !s_gapWaiting)
    {
        // Nothing in flight — transmit immediately
        buildRmtSymbols(timings, count);
        if (s_rmtSymbolCount > 0)
            startTransmit(gap);
    }
    else if (!s_hasPending)
    {
        // Queue for after current transmission + gap
        memcpy(s_pendingTimings, timings, count * sizeof(uint16_t));
        s_pendingCount = count;
        s_pendingGap   = gap;
        s_hasPending   = true;
    }
    else
    {
        LOG("IR TX: busy, dropping transmission\n");
    }
}

// ---- IR Packet Handler ----
// [uint16 cmd=1][uint16 reserved][uint32 carrierFreq][uint32 gap][uint16 timings...]
void handleIrPacket(uint8_t* data, int length)
{
    if (length < IR_HEADER_SIZE + 2)
    {
        LOG("IR: packet too short\n");
        return;
    }

    uint32_t carrierFreq;
    uint32_t gap;
    memcpy(&carrierFreq, data + 4, 4);
    memcpy(&gap,         data + 8, 4);

    if (carrierFreq != 38000)
    {
        LOG("IR: unsupported carrier frequency\n");
        return;
    }

    int timingBytes = length - IR_HEADER_SIZE;
    int timingCount = timingBytes / 2;

    // Local buffer needed: transmitTimingsNoLog may write one padding element
    uint16_t timings[MAX_TIMING_VALUES + 1];
    memcpy(timings, data + IR_HEADER_SIZE, timingBytes);

    VERBOSE("IR TX: timings: [%i] gap: %i\n", timingCount, gap);
    // Raw timing packets don't carry a protocol+code — clear whichever slot they occupy.
    if (!s_rmtBusy && !s_gapWaiting)
        s_inflightCodeValid = false;
    else if (!s_hasPending)
        s_pendingCodeValid = false;
    transmitTimingsNoLog(timings, timingCount, gap);
}

// ---- IR Code Handler ----
// Encodes a value using a named protocol and transmits it.
// Packet layout: [uint16 cmd][uint16 reserved][uint32 protocol][uint64 code][uint8 repeat]
#define IR_CODE_PACKET_SIZE 17
void handleIrCodePacket(uint8_t* data, int length)
{
    if (length < IR_CODE_PACKET_SIZE)
    {
        LOG("IR: code packet too short\n");
        return;
    }

    uint32_t protocolId;
    uint64_t code;
    uint8_t  repeat;
    memcpy(&protocolId,  data + 4,  4);
    memcpy(&code,        data + 8,  8);
    memcpy(&repeat,      data + 16, 1);

    handleIrCode(protocolId, code, (bool)repeat);
}

void handleIrCode(uint32_t protocolId, uint64_t code, bool repeat)
{
    const IrProtocol* protocol = getIrProtocolById(protocolId);
    if (!protocol)
    {
        LOG("IR: unknown protocol 0x%08X\n", protocolId);
        return;
    }

    uint16_t timings[MAX_TIMING_VALUES + 1];
    int count = 0;
    int gap   = 0;
    ir_encode(*protocol, code, repeat, timings, &count, &gap);

    if (count <= 0)
    {
        LOG("IR: encode produced no timings\n");
        return;
    }

    VERBOSE("IR TX: protocol: 0x%08X code: 0x%016llX repeat: %i\n", protocolId, code, repeat ? 1 : 0);
    // Track which slot this occupies so getInflightIrCode() can report what the TV is receiving.
    if (!s_rmtBusy && !s_gapWaiting)
    {
        s_inflightCodeValid = true;
        s_inflightProtocol  = protocolId;
        s_inflightCode      = code;
    }
    else if (!s_hasPending)
    {
        s_pendingCodeValid = true;
        s_pendingProtocol  = protocolId;
        s_pendingCode      = code;
    }
    transmitTimingsNoLog(timings, count, (uint32_t)gap);
}
