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
#include "ir_decoder.h"
#include "ir_router.h"
#include "activities.h"

// ── Configuration ─────────────────────────────────────────────────────────────

#define RMT_RES_HZ          1000000U    // 1 MHz clock → 1 µs per RMT tick
#define IDLE_THRESHOLD_US   15000U      // 15 ms idle ends a frame (> any intra-frame gap)
#define BUF_SYMBOLS         96          // 2 × 48-word hw blocks (both RX channels' memory)
#define LONG_BREAK_US       5000000ULL  // threshold for "long break" label (5 s)

// ── Globals ───────────────────────────────────────────────────────────────────

static rmt_channel_handle_t rx_chan  = NULL;
static rmt_symbol_word_t    rx_buf[BUF_SYMBOLS];
static QueueHandle_t        rx_queue;

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

void onDecoded(const IrProtocol& protocol, uint64_t data) {
    VERBOSE("Decoded %s: 0x%016llX\n", protocol.name, data);
    applyActivityBinding(protocol.id, data);
    applyRoutes(protocol.id, data);
}


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
    cfg.signal_range_min_ns     = 1000U;                       // 1 µs glitch filter
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

    static int64_t last_end_us = -1;

    rmt_rx_done_event_data_t edata;
    if (xQueueReceive(rx_queue, &edata, 0) != pdTRUE) return;

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
        if (syms[i].duration0 > 0)
            decode_pulse(syms[i].level0 == 0, syms[i].duration0);
        // duration1 == 0 on the final symbol means idle timeout fired here — skip it.
        if (syms[i].duration1 > 0)
            decode_pulse(syms[i].level1 == 0, syms[i].duration1);
    }

    last_end_us = esp_timer_get_time();
    start_receive();
}
