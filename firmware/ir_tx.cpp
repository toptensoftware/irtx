#include <Arduino.h>
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "config.h"
#include "led.h"
#include "ir_tx.h"
#include "ir_protocol.h"
#include "ir_encoder.h"

// ---- RMT Globals ----
static rmt_channel_handle_t txChannel   = NULL;
static rmt_encoder_handle_t copyEncoder = NULL;

// ---- Slot state ----
enum SlotAState { SLOT_A_FREE, SLOT_A_RMT_ACTIVE, SLOT_A_GAP_WAIT };
enum SlotBState { SLOT_B_FREE, SLOT_B_PENDING };

struct IrTxSlot
{
    uint16_t          timings[MAX_TIMING_VALUES + 1];
    rmt_symbol_word_t symbols[MAX_TIMING_VALUES];
    int               symbolCount;
    uint32_t          gap;
    int64_t           gapExpiresAt;
};

static IrTxSlot   s_slotA;
static IrTxSlot   s_slotB;
static SlotAState s_slotAState = SLOT_A_FREE;
static SlotBState s_slotBState = SLOT_B_FREE;

// ---- Scratch buffer for incoming packet data ----
static uint16_t s_timingScratch[MAX_TIMING_VALUES + 1];

// ---- Build RMT Symbols into a slot ----
static void buildRmtSymbols(IrTxSlot& slot, int timingCount)
{
    struct Phase { uint8_t level; uint16_t duration; };
    static Phase phases[MAX_TIMING_VALUES * 2];
    int phaseCount = 0;

    for (int i = 0; i < timingCount; i++)
    {
        uint8_t level = (i % 2 == 0) ? 1 : 0;
        uint16_t remaining = slot.timings[i];
        while (remaining > 0 && phaseCount < MAX_TIMING_VALUES * 2)
        {
            uint16_t chunk = (remaining > 32767) ? 32767 : remaining;
            phases[phaseCount++] = {level, chunk};
            remaining -= chunk;
        }
    }

    slot.symbolCount = 0;
    for (int i = 0; i < phaseCount; i += 2)
    {
        slot.symbols[slot.symbolCount].level0    = phases[i].level;
        slot.symbols[slot.symbolCount].duration0 = phases[i].duration;
        if (i + 1 < phaseCount)
        {
            slot.symbols[slot.symbolCount].level1    = phases[i+1].level;
            slot.symbols[slot.symbolCount].duration1 = phases[i+1].duration;
        }
        else
        {
            slot.symbols[slot.symbolCount].level1    = 0;
            slot.symbols[slot.symbolCount].duration1 = 0;
        }
        slot.symbolCount++;
    }
}

// ---- Start transmitting slot A via RMT (non-blocking) ----
static void startSlotA()
{
    rmt_transmit_config_t txCfg = {};
    txCfg.loop_count = 0;
    esp_err_t err = rmt_transmit(txChannel, copyEncoder,
                                 s_slotA.symbols,
                                 s_slotA.symbolCount * sizeof(rmt_symbol_word_t),
                                 &txCfg);
    if (err != ESP_OK)
    {
        LOG("IR: transmit error: %s\n", esp_err_to_name(err));
        s_slotAState = SLOT_A_FREE;
        return;
    }
    setLed(LED_PRIORITY_ACTIVITY, 0x004000);    // Bright green
    s_slotAState = SLOT_A_RMT_ACTIVE;
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

// ---- Poll ----
void pollIrTx()
{
    if (txChannel == NULL) return;

    // Check if RMT hardware has finished
    if (s_slotAState == SLOT_A_RMT_ACTIVE)
    {
        if (rmt_tx_wait_all_done(txChannel, 0) == ESP_OK)
        {
            s_slotA.gapExpiresAt = esp_timer_get_time() + s_slotA.gap;
            s_slotAState = SLOT_A_GAP_WAIT;
            setLed(LED_PRIORITY_ACTIVITY, 0xFFFFFFFF);
        }
    }

    // Check if inter-packet gap has expired
    if (s_slotAState == SLOT_A_GAP_WAIT)
    {
        if (esp_timer_get_time() >= s_slotA.gapExpiresAt)
            s_slotAState = SLOT_A_FREE;
    }

    // Promote pending slot once slot A is free
    if (s_slotAState == SLOT_A_FREE && s_slotBState == SLOT_B_PENDING)
    {
        s_slotA = s_slotB;
        s_slotBState = SLOT_B_FREE;
        startSlotA();
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

    if (s_slotAState == SLOT_A_FREE)
    {
        memcpy(s_slotA.timings, timings, count * sizeof(uint16_t));
        s_slotA.gap = gap;
        buildRmtSymbols(s_slotA, count);
        if (s_slotA.symbolCount == 0) return;
        startSlotA();
    }
    else if (s_slotBState == SLOT_B_FREE)
    {
        memcpy(s_slotB.timings, timings, count * sizeof(uint16_t));
        s_slotB.gap = gap;
        buildRmtSymbols(s_slotB, count);
        if (s_slotB.symbolCount == 0) return;
        s_slotBState = SLOT_B_PENDING;
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
    memcpy(s_timingScratch, data + IR_HEADER_SIZE, timingBytes);

    VERBOSE("IR TX: timings: [%i] gap: %i\n", timingCount, gap);
    transmitTimingsNoLog(s_timingScratch, timingCount, gap);
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

    int count = 0;
    int gap   = 0;
    ir_encode(*protocol, code, repeat, s_timingScratch, &count, &gap);

    if (count <= 0)
    {
        LOG("IR: encode produced no timings\n");
        return;
    }

    VERBOSE("IR TX: protocol: 0x%08X code: 0x%016llX repeat: %i\n", protocolId, code, repeat ? 1 : 0);
    transmitTimingsNoLog(s_timingScratch, count, (uint32_t)gap);
}
