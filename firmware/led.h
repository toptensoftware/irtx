#pragma once

#define LED_PRIORITY_CONNECTIVITY 0
#define LED_PRIORITY_USER         1
#define LED_PRIORITY_UNUSED       2
#define LED_PRIORITY_ACTIVITY     3

void setLed(int priority, uint32_t color);