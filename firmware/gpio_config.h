#pragma once

#include <stdint.h>

#define MAX_GPIO_PULL_PINS 16
#define GPIO_PIN_NONE      0xFF

struct GpioPinSlot {
    uint8_t pinA;
    uint8_t pinB;   // GPIO_PIN_NONE if single-pin slot
};

extern int          gpioIrTxPin;
extern int          gpioIrRxPin;
extern int          gpioLedPin;
extern GpioPinSlot  gpioPullupSlots[MAX_GPIO_PULL_PINS];
extern uint8_t      gpioPullupCount;
extern GpioPinSlot  gpioPulldownSlots[MAX_GPIO_PULL_PINS];
extern uint8_t      gpioPulldownCount;

// Override compile-time pin defaults with runtime-configurable variables.
// Any file that needs the actual runtime pin must include this header.
#define IR_TX_PIN gpioIrTxPin
#define IR_RX_PIN gpioIrRxPin

void setupGpioConfig();
void statusGpioConfig();
void gpioSetPin(int pinA, int pinB, const char* func);

void pollGpio();

// Callbacks — weak stubs are provided; override in application code.
void onButton(int pin, bool pressed);
void onEncoder(int pin, int direction, uint32_t velocity);
