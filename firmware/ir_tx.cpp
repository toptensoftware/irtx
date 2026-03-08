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

// ---- Device State ----
struct IrDeviceState { int64_t availableTime; };
static IrDeviceState irDeviceStates[MAX_IR_DEVICES] = {};

// ---- Buffers ----
static uint16_t          timingValues[MAX_TIMING_VALUES + 1];
static rmt_symbol_word_t rmtSymbols[MAX_TIMING_VALUES];
static int               rmtSymbolCount = 0;

// ---- RMT Setup ----
void setupIrTx()
{
    if (IR_RX_PIN < 0) {
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

    rmtSymbolCount = 0;
    for (int i = 0; i < phaseCount; i += 2)
    {
        rmtSymbols[rmtSymbolCount].level0    = phases[i].level;
        rmtSymbols[rmtSymbolCount].duration0 = phases[i].duration;
        if (i + 1 < phaseCount)
        {
            rmtSymbols[rmtSymbolCount].level1    = phases[i+1].level;
            rmtSymbols[rmtSymbolCount].duration1 = phases[i+1].duration;
        }
        else
        {
            rmtSymbols[rmtSymbolCount].level1    = 0;
            rmtSymbols[rmtSymbolCount].duration1 = 0;
        }
        rmtSymbolCount++;
    }
}

// ---- Common Transmit Helper ----
static void transmitTimings(uint16_t irDevIdx, uint16_t* timings, int count, uint32_t gap)
{
    // Make sure even count
    if (count % 2 != 0)
    {
        timings[count] = 0;
        count++;
    }

    // Sum up the total length
    int timingLength = 0;
    for (int i = 0; i < count; i++)
        timingLength += timings[i];

    // Enforce per-device gap between transmissions
    IrDeviceState* dev = &irDeviceStates[irDevIdx];
    int64_t now = esp_timer_get_time();
    if (dev->availableTime > 0 && now < dev->availableTime)
        delayMicroseconds(dev->availableTime - now);
    dev->availableTime = now + timingLength + gap;

    // Setup RMT
    buildRmtSymbols(timings, count);
    if (rmtSymbolCount == 0) return;

    // Show activity
    ledColor(0, 64, 0);

    // Transmit
    rmt_transmit_config_t txCfg = {};
    txCfg.loop_count = 0;
    esp_err_t err = rmt_transmit(txChannel, copyEncoder,
                                 rmtSymbols, rmtSymbolCount * sizeof(rmt_symbol_word_t),
                                 &txCfg);
    if (err != ESP_OK)
    {
        LOG("IR: transmit error: %s\n", esp_err_to_name(err));
        ledColor(4, 0, 0);
        return;
    }
    rmt_tx_wait_all_done(txChannel, portMAX_DELAY);

    // Clear activity
    ledColor(0, 2, 0);
}

// ---- IR Packet Handler ----
// [uint16 cmd=1][uint16 irDevIdx][uint32 carrierFreq][uint32 gap][uint16 timings...]
void handleIrPacket(uint8_t* data, int length)
{
    if (length < IR_HEADER_SIZE + 2)
    {
        LOG("IR: packet too short\n");
        return;
    }

    uint16_t irDevIdx;
    uint32_t carrierFreq;
    uint32_t gap;
    memcpy(&irDevIdx,    data + 2, 2);
    memcpy(&carrierFreq, data + 4, 4);
    memcpy(&gap,         data + 8, 4);

    if (irDevIdx >= MAX_IR_DEVICES)
    {
        LOG("IR: device index out of range\n");
        return;
    }

    if (carrierFreq != 38000)
    {
        LOG("IR: unsupported carrier frequency\n");
        return;
    }

    int timingBytes = length - IR_HEADER_SIZE;
    int timingCount = timingBytes / 2;
    memcpy(timingValues, data + IR_HEADER_SIZE, timingBytes);

    transmitTimings(irDevIdx, timingValues, timingCount, gap);
}

// ---- IR Code Handler ----
// Encodes a value using a named protocol and transmits it.
// Packet layout: [uint16 cmd][uint16 devIdx][uint32 protocol][uint64 code][uint8 repeat]
#define IR_CODE_PACKET_SIZE 17
void handleIrCodePacket(uint8_t* data, int length)
{
    if (length < IR_CODE_PACKET_SIZE)
    {
        LOG("IR: code packet too short\n");
        return;
    }

    uint16_t irDevIdx;
    uint32_t protocolId;
    uint64_t code;
    uint8_t  repeat;
    memcpy(&irDevIdx,    data + 2,  2);
    memcpy(&protocolId,  data + 4,  4);
    memcpy(&code,        data + 8,  8);
    memcpy(&repeat,      data + 16, 1);

    handleIrCode(irDevIdx, protocolId, code, (bool)repeat);
}

void handleIrCode(uint16_t irDevIdx, uint32_t protocolId, uint64_t code, bool repeat)
{
    if (irDevIdx >= MAX_IR_DEVICES)
    {
        LOG("IR: device index out of range\n");
        return;
    }

    const IrProtocol* protocol = getIrProtocolById(protocolId);
    if (!protocol)
    {
        LOG("IR: unknown protocol 0x%08X\n", protocolId);
        return;
    }

    int count = 0;
    int gap   = 0;
    ir_encode(*protocol, code, repeat, timingValues, &count, &gap);

    if (count <= 0)
    {
        LOG("IR: encode produced no timings\n");
        return;
    }

    transmitTimings(irDevIdx, timingValues, count, (uint32_t)gap);
}
