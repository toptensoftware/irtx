#pragma once

#include <Preferences.h>

// ---- Logging ----
// Operational log messages prefixed with seconds since boot.
#define LOG(fmt, ...) Serial.printf("[%7.3f] " fmt, millis() / 1000.0f, ##__VA_ARGS__)

// ---- Hardware ----
#define UDP_PORT     4210
#ifdef CONFIG_IDF_TARGET_ESP32C6
#define IR_TX_PIN    11
#define IR_RX_PIN    10
#define LED_PIN      8        // Onboard WS2812 RGB LED
#else
#define IR_TX_PIN    4
#define IR_RX_PIN    -1
#define LED_PIN      10       // Onboard WS2812 RGB LED
#endif
#define CARRIER_FREQ 38000   // 38kHz IR carrier

// ---- Limits ----
#define MAX_TIMING_VALUES  512
#define MAX_IR_DEVICES     16
#define IR_HEADER_SIZE     12  // uint16 cmd + uint16 irDevIdx + uint32 carrierFreq + uint32 gap
#define INPUT_MAX          256

extern Preferences prefs;

void nvsDump();
void nvsReset();