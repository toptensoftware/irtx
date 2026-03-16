#include <Arduino.h>
#include "config.h"


static uint32_t colors[] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, };
static int current_priority = -1;

void setLed(int priority, uint32_t color)
{
    // Check in range
    if (priority < 0 || priority >=(sizeof(colors) / sizeof(colors[0])))
        return;

    // Quit if redundant
    if (colors[priority] == color)
        return; 

    // Store color
    colors[priority] = color;

    // Update led
    if (priority >= current_priority)
    {
        // If clearing this priority level, then find the next highest
        if (color == 0xFFFFFFFF)
        {
            current_priority = -1;
            color = 0;
            for (int p = priority - 1; p >= 0; p--)
            {
                if (colors[p] != 0xFFFFFFFF)
                {
                    current_priority = p;
                    color = colors[p];
                    break;
                }
            }
        }
        else
        {
            // New highest priority
            current_priority = priority;
        }

        // Unused?
        if (color == 0xFFFFFFFF)
            color = 0;

    #ifdef CONFIG_IDF_TARGET_ESP32C6
        neopixelWrite(LED_PIN, (color >> 8) & 0xFF, (color >> 16) & 0xFF, color & 0xFF);
    #else
        neopixelWrite(LED_PIN, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
    #endif
    }
}