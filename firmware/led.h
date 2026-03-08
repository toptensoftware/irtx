#pragma once

#include <Arduino.h>
#include "config.h"

// ---- Status LED (onboard WS2812) ----
inline void ledColor(uint8_t r, uint8_t g, uint8_t b)
{
#ifdef CONFIG_IDF_TARGET_ESP32C6
    neopixelWrite(LED_PIN, g, r, b);  // LED on C6 board uses RGB order instead of GRB
#else
    neopixelWrite(LED_PIN, r, g, b);
#endif
}
