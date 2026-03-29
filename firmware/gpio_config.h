#pragma once

#include <stdint.h>

#define MAX_GPIO_PULL_PINS 16

extern int     gpioIrTxPin;
extern int     gpioIrRxPin;
extern uint8_t gpioPullupPins[MAX_GPIO_PULL_PINS];
extern uint8_t gpioPullupCount;
extern uint8_t gpioPulldownPins[MAX_GPIO_PULL_PINS];
extern uint8_t gpioPulldownCount;

// Override compile-time pin defaults with runtime-configurable variables.
// Any file that needs the actual runtime pin must include this header.
#define IR_TX_PIN gpioIrTxPin
#define IR_RX_PIN gpioIrRxPin

void setupGpioConfig();
void statusGpioConfig();
void gpioSetPin(int pin, const char* func);
