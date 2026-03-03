#pragma once

#include <Arduino.h>
#include "config.h"

// ---- Status LED (onboard WS2812) ----
inline void ledColor(uint8_t r, uint8_t g, uint8_t b)
{
    neopixelWrite(LED_PIN, r, g, b);
}
