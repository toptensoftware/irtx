#pragma once

#include <Preferences.h>

// ---- Logging ----
// Operational log messages prefixed with seconds since boot.
#define LOG(fmt, ...) Serial.printf("[%7.3f] " fmt, millis() / 1000.0f, ##__VA_ARGS__)

// ---- Hardware ----
#define UDP_PORT     4210
#define IR_TX_PIN    4
#define LED_PIN      10       // Onboard WS2812 RGB LED
#define CARRIER_FREQ 38000   // 38kHz IR carrier

// ---- Limits ----
#define MAX_TIMING_VALUES  512
#define MAX_IR_DEVICES     16
#define IR_HEADER_SIZE     12  // uint16 cmd + uint16 irDevIdx + uint32 carrierFreq + uint32 gap
#define INPUT_MAX          256

extern Preferences prefs;
