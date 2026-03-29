//
// ir_rx — Demodulated IR receiver timing logger for ESP32
//
// Connects a demodulated IR receiver (e.g. TSOP38238, VS1838B) to GPIO 
//
// Wiring
// ------
//   IR receiver OUT  →  GPIO
//   IR receiver VCC  →  3.3 V
//   IR receiver GND  →  GND
//
// Uses the ESP32 RMT peripheral in receive mode (ESP-IDF v5 / arduino-esp32 v3.x).
// Resolution: 1 µs per RMT tick.
//
// Each RMT symbol packs two consecutive level/duration pairs. Frames are
// delimited by IDLE_THRESHOLD_US of silence on the line, after which RMT
// delivers all captured symbols to the main loop via a FreeRTOS queue.
//
// Inter-frame gaps are printed as SPACE entries; the duration is measured
// using esp_timer_get_time() in loop() (accurate to within a few ms).
//

#include <Arduino.h>
#include "driver/rmt_rx.h"
#include "esp_timer.h"
#include "config.h"
#include "gpio_config.h"
#include "ir_decoder.h"
#include "activities.h"
#include "ir_rx.h"

// ── Configuration ─────────────────────────────────────────────────────────────

#define RMT_RES_HZ          1000000U    // 1 MHz clock → 1 µs per RMT tick
#define IDLE_THRESHOLD_US   15000U      // 15 ms idle ends a frame (> any intra-frame gap)
#define BUF_SYMBOLS         96          // 2 × 48-word hw blocks (both RX channels' memory)
#define LONG_BREAK_US       5000000ULL  // threshold for "long break" label (5 s)

#define IR_REPEAT_CODE      0xFFFFFFFFFFFFFFFFULL
#define PRESS_TIMEOUT_US    250000LL    // 200 ms — same code within this window is a repeat
#define LONG_PRESS_US       500000LL    // 500 ms since press → synthesize long-press

// ── Globals ───────────────────────────────────────────────────────────────────

static rmt_channel_handle_t rx_chan  = NULL;
static rmt_symbol_word_t    rx_buf[BUF_SYMBOLS];
static QueueHandle_t        rx_queue;

// ── IR event synthesis state ──────────────────────────────────────────────────

static struct {
    uint32_t protocol_id;
    uint64_t code;
    int64_t  press_us;          // when the press event was fired
    int64_t  last_received_us;  // when the last same-code was received
    bool     long_press_fired;
    bool     active;
    bool     suppress_release;
} ir_state = {};

// ── Synthetic repeat state ────────────────────────────────────────────────────

static int64_t s_syntheticRepeatRate_us = 0;  // 0 = use natural IR repeat rate
static int64_t s_lastSyntheticFire_us       = 0;

// ── IR event stub ─────────────────────────────────────────────────────────────

const char* nameOfEventKind(IrEventKind kind)
{
    switch (kind)
    {
        case IrEventKind::Press: return "press";
        case IrEventKind::Repeat: return "repeat";
        case IrEventKind::LongPress: return "long-press";
        case IrEventKind::Release: return "release";
    }
}

void onIrEvent(uint32_t protocol_id, uint64_t code, IrEventKind kind)
{
//    if (kind != IrEventKind::Repeat)
//        VERBOSE("IR Event %s 0x%08X/%016llX\n", nameOfEventKind(kind), protocol_id, code);
    invokeBindings(protocol_id, code, kind);
}

// ── ISR callback ──────────────────────────────────────────────────────────────
//
// Called by the RMT driver (in ISR context) when an idle timeout ends a frame.
// Posts the event to the queue; loop() drains it.
//
// Queue depth is 1 because received_symbols points into the shared rx_buf —
// the next rmt_receive() must not be issued until the previous frame is consumed.

static bool IRAM_ATTR rx_done_cb(rmt_channel_handle_t          chan,
                                  const rmt_rx_done_event_data_t *edata,
                                  void                           *user_ctx)
{
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR((QueueHandle_t)user_ctx, edata, &woken);
    return woken == pdTRUE;
}

// ── Helpers ───────────────────────────────────────────────────────────────────

bool didDecode = false;
void onDecoded(uint32_t protocol, uint64_t data)
{
    didDecode = true;
    VERBOSE("IR Decoded 0x%08X/%016llX\n", protocol, data);

    int64_t now = esp_timer_get_time();

    // Normalise repeat indicator: treat it as the last code for this protocol.
    // If there's no matching active press, ignore the repeat packet entirely.
    uint64_t code = data;
    if (data == IR_REPEAT_CODE) {
        if (!ir_state.active || ir_state.protocol_id != protocol)
            return;
        code = ir_state.code;
    }

    // Is this the same key as the one currently held, within the repeat window?
    bool same_code     = ir_state.active &&
                         ir_state.protocol_id == protocol &&
                         ir_state.code == code;
    bool within_window = same_code &&
                         (now - ir_state.last_received_us) < PRESS_TIMEOUT_US;

    if (within_window) {
        // ── Repeat ────────────────────────────────────────────────────────────
        ir_state.last_received_us = now;

        // In synthetic repeat mode, real repeat frames just keep the key alive;
        // the poll loop fires the Repeat events at the configured interval instead.
        if (s_syntheticRepeatRate_us == 0) {
            onIrEvent(protocol, ir_state.code, IrEventKind::Repeat);

            // Long press fires once, after 500 ms of continuous holding
            if (!ir_state.long_press_fired &&
                (now - ir_state.press_us) >= LONG_PRESS_US) {
                ir_state.long_press_fired = true;
                onIrEvent(protocol, ir_state.code, IrEventKind::LongPress);
            }
        }
    } else {
        // ── Release old key (if any), then press the new one ──────────────────
        if (ir_state.active) {
            if (!ir_state.suppress_release)
                onIrEvent(ir_state.protocol_id, ir_state.code, IrEventKind::Release);
            ir_state.active            = false;
            ir_state.suppress_release  = false;
        }

        ir_state.protocol_id       = protocol;
        ir_state.code              = code;
        ir_state.press_us          = now;
        ir_state.last_received_us  = now;
        ir_state.long_press_fired  = false;
        ir_state.suppress_release  = false;
        ir_state.active            = true;

        onIrEvent(protocol, code, IrEventKind::Press);
    }
}

void onDecoded(const IrProtocol& protocol, uint64_t data)
{
    onDecoded(protocol.id, data);
}

// ── Pulse glitch accumulator ──────────────────────────────────────────────────
/*
// Short pulses (< 200 µs) are noise from the demodulated receiver.  Rather than
// dropping them, we accumulate consecutive short pulses and emit them as one
// combined pulse so the total duration is preserved.  E.g. three glitch pulses
// of 45+14+33 µs become a single 92 µs space — which after jitter correction
// (+300) gives 392 µs, matching the expected short space of ~387 µs.
static bool     s_glitchPulse = false;
static uint64_t s_glitchUs    = 0;

static void decode_pulse(bool pulse, uint64_t us)
{
    if (us < 200) 
    {
        if (s_glitchUs == 0)
            s_glitchPulse = pulse;  // first glitch determines the combined direction
        s_glitchUs += us;
        return;
    }

    // Flush any accumulated glitch as one combined pulse before processing this one
    if (s_glitchUs > 0) 
    {
        int64_t adj = (int64_t)s_glitchUs + (s_glitchPulse ? -300LL : 300LL);
        if (adj > 0)
            ir_decode(s_glitchPulse, (int)adj);
        s_glitchUs = 0;
    }

    // Adjust for demodulated receiver's jitter and decode
    int64_t adj = (int64_t)us + (pulse ? -300LL : 300LL);
    if (adj > 0)
        ir_decode(pulse, (int)adj);
}
*/

static void decode_pulse(bool pulse, uint64_t us)
{
    // Adjust for demodulated receiver's jitter
    us += pulse ? -300 : 300;

    // Decode
    ir_decode(pulse, (int)us);
}

static void start_receive(void)
{
    rmt_receive_config_t cfg    = {};
    cfg.signal_range_min_ns     = 1000U;                       // 1 µs (hardware max is ~3 µs; glitch filtering done in software)
    cfg.signal_range_max_ns     = IDLE_THRESHOLD_US * 1000U;   // 15 ms idle → done
    ESP_ERROR_CHECK(rmt_receive(rx_chan, rx_buf, sizeof(rx_buf), &cfg));
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void setupIrRx()
{
    if (IR_RX_PIN < 0) {
        LOG("IR RX disabled (no pin assigned)\n");
        return;
    }

    // Initialize all decoders
    ir_decode_init(onDecoded);

    // Queue depth 1 — rx_buf is shared; only one frame may be pending at a time.
    rx_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));

    // Configure RMT RX channel
    rmt_rx_channel_config_t rx_cfg = {};
    rx_cfg.gpio_num          = (gpio_num_t)IR_RX_PIN;
    rx_cfg.clk_src           = RMT_CLK_SRC_DEFAULT;
    rx_cfg.resolution_hz     = RMT_RES_HZ;
    rx_cfg.mem_block_symbols = BUF_SYMBOLS;

    ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_cfg, &rx_chan));

    rmt_rx_event_callbacks_t cbs = {};
    cbs.on_recv_done = rx_done_cb;
    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(rx_chan, &cbs, rx_queue));

    ESP_ERROR_CHECK(rmt_enable(rx_chan));
    start_receive();

    LOG("RMT RX initialized on GPIO %d\n", IR_RX_PIN);

}

// ── Poll loop ─────────────────────────────────────────────────────────────────
//
// RMT delivers frames as a contiguous array of rmt_symbol_word_t, each holding
// two (level, duration) pairs:
//
//   symbol.level0 / .duration0  →  first  period
//   symbol.level1 / .duration1  →  second period
//
// For a demodulated receiver: LOW (level=0) = mark, HIGH (level=1) = space.
// The last symbol has duration1 == 0 when the frame ended by idle timeout
// rather than by a real transition — we skip it to avoid a spurious zero entry.

void pollIrRx()
{
    if (IR_RX_PIN < 0)
        return;

    // Synthesise a release if no same-code has arrived within the repeat window;
    // otherwise fire synthetic repeats at the configured interval if set.
    if (ir_state.active) {
        int64_t now = esp_timer_get_time();
        if ((now - ir_state.last_received_us) >= PRESS_TIMEOUT_US) {
            if (!ir_state.suppress_release)
                onIrEvent(ir_state.protocol_id, ir_state.code, IrEventKind::Release);
            ir_state.active              = false;
            ir_state.suppress_release    = false;
            s_syntheticRepeatRate_us = 0;
        } else if (s_syntheticRepeatRate_us > 0) {
            if (now - s_lastSyntheticFire_us >= s_syntheticRepeatRate_us) {
                s_lastSyntheticFire_us = now;
                onIrEvent(ir_state.protocol_id, ir_state.code, IrEventKind::Repeat);
            }
        }
    }

    static int64_t last_end_us = -1;

    rmt_rx_done_event_data_t edata;
    if (xQueueReceive(rx_queue, &edata, 0) != pdTRUE) return;

    didDecode = false;

    // Inter-frame gap: time elapsed since we finished the last frame.
    // Measured in loop() so it includes RMT restart + scheduling latency (~1 ms),
    // but is accurate enough for IR analysis of typical remote controls.
    if (last_end_us >= 0) {
        uint64_t gap = (uint64_t)(esp_timer_get_time() - last_end_us);
        decode_pulse(false, gap);
    }

    // Print mark/space timing for this frame.
    rmt_symbol_word_t *syms = edata.received_symbols;
    size_t n = edata.num_symbols;

    for (size_t i = 0; i < n; i++) {

        if (i + 1 < n)
        {
            // Glitch pulse with incorrect space in it (short space and short next pulse)
            if (syms[i].duration1 < 40)
            {
                syms[i+1].duration0 += syms[i].duration0 + syms[i].duration1;
                continue;
            }
        }

        if (syms[i].duration0 > 0)
            decode_pulse(syms[i].level0 == 0, syms[i].duration0);

        // duration1 == 0 on the final symbol means idle timeout fired here — skip it.
        if (syms[i].duration1 > 0)
            decode_pulse(syms[i].level1 == 0, syms[i].duration1);
    }

    if (!didDecode && n > 50)
    {
        VERBOSE("FAILED: IR packet: (%i)\n", n);
        for (size_t i = 0; i < n; i++)
        {
            VERBOSE("%c%i %c%i\n", 
                syms[i].level0 == 0 ? 'p' : 's',
                syms[i].duration0, 
                syms[i].level1 == 0 ? 'p' : 's',
                syms[i].duration1);
        }
    }

    last_end_us = esp_timer_get_time();
    start_receive();
}


void setIrRepeatRate(uint32_t ms)
{
    s_syntheticRepeatRate_us = (int64_t)ms * 1000LL;
    s_lastSyntheticFire_us       = esp_timer_get_time();
}

void suppressRelease()
{
    ir_state.suppress_release = true;
}

void simulateIrRx(uint32_t protocol, uint64_t code, uint32_t eventKindMask)
{
    if (eventKindMask == 0)
    {
        onDecoded(protocol, code);
    }
    else
    {
        onIrEvent(protocol, code, (IrEventKind)eventKindMask);
    }
}

