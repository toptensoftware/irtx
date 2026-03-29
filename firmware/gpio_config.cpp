#include <Arduino.h>
#include "config.h"
#include "gpio_config.h"

int     gpioIrTxPin   = IR_TX_PIN_DEFAULT;
int     gpioIrRxPin   = IR_RX_PIN_DEFAULT;
uint8_t gpioPullupPins[MAX_GPIO_PULL_PINS]   = {};
uint8_t gpioPullupCount  = 0;
uint8_t gpioPulldownPins[MAX_GPIO_PULL_PINS] = {};
uint8_t gpioPulldownCount = 0;

static void removePinFromAllRoles(int pin)
{
    if (gpioIrTxPin == pin) gpioIrTxPin = -1;
    if (gpioIrRxPin == pin) gpioIrRxPin = -1;

    for (int i = 0; i < gpioPullupCount; i++)
    {
        if (gpioPullupPins[i] == (uint8_t)pin)
        {
            memmove(&gpioPullupPins[i], &gpioPullupPins[i + 1], gpioPullupCount - i - 1);
            gpioPullupCount--;
            break;
        }
    }
    for (int i = 0; i < gpioPulldownCount; i++)
    {
        if (gpioPulldownPins[i] == (uint8_t)pin)
        {
            memmove(&gpioPulldownPins[i], &gpioPulldownPins[i + 1], gpioPulldownCount - i - 1);
            gpioPulldownCount--;
            break;
        }
    }
}

static void saveToNvs()
{
    extern Preferences prefs;
    prefs.begin("gpio", false);
    prefs.putInt("irtx", gpioIrTxPin);
    prefs.putInt("irrx", gpioIrRxPin);
    prefs.putBytes("pullup",   gpioPullupPins,   gpioPullupCount);
    prefs.putBytes("pulldown", gpioPulldownPins, gpioPulldownCount);
    prefs.end();
}

void setupGpioConfig()
{
    extern Preferences prefs;
    prefs.begin("gpio", true);
    gpioIrTxPin      = prefs.getInt("irtx", IR_TX_PIN_DEFAULT);
    gpioIrRxPin      = prefs.getInt("irrx", IR_RX_PIN_DEFAULT);
    gpioPullupCount  = (uint8_t)prefs.getBytes("pullup",   gpioPullupPins,   MAX_GPIO_PULL_PINS);
    gpioPulldownCount = (uint8_t)prefs.getBytes("pulldown", gpioPulldownPins, MAX_GPIO_PULL_PINS);
    prefs.end();

    for (int i = 0; i < gpioPullupCount; i++)
        pinMode(gpioPullupPins[i], INPUT_PULLUP);
    for (int i = 0; i < gpioPulldownCount; i++)
        pinMode(gpioPulldownPins[i], INPUT_PULLDOWN);

    LOG("GPIO config: irtx=%d irrx=%d pullup=%d pulldown=%d\n",
        gpioIrTxPin, gpioIrRxPin, gpioPullupCount, gpioPulldownCount);
}

void statusGpioConfig()
{
    PRINT("IR TX pin: %d\n", gpioIrTxPin);
    PRINT("IR RX pin: %d\n", gpioIrRxPin);

    PRINT("Pullup pins (%d):", gpioPullupCount);
    if (gpioPullupCount == 0)
        PRINT(" (none)");
    for (int i = 0; i < gpioPullupCount; i++)
        PRINT(" %d", gpioPullupPins[i]);
    PRINT("\n");

    PRINT("Pulldown pins (%d):", gpioPulldownCount);
    if (gpioPulldownCount == 0)
        PRINT(" (none)");
    for (int i = 0; i < gpioPulldownCount; i++)
        PRINT(" %d", gpioPulldownPins[i]);
    PRINT("\n");
}

void gpioSetPin(int pin, const char* func)
{
    if (pin < 0)
    {
        PRINT("Invalid pin: %d\n", pin);
        return;
    }

    removePinFromAllRoles(pin);

    if (strcmp(func, "irtx") == 0)
    {
        gpioIrTxPin = pin;
    }
    else if (strcmp(func, "irrx") == 0)
    {
        gpioIrRxPin = pin;
    }
    else if (strcmp(func, "pullup") == 0)
    {
        if (gpioPullupCount >= MAX_GPIO_PULL_PINS)
        {
            PRINT("Error: max %d pullup pins already configured\n", MAX_GPIO_PULL_PINS);
            return;
        }
        gpioPullupPins[gpioPullupCount++] = (uint8_t)pin;
    }
    else if (strcmp(func, "pulldown") == 0)
    {
        if (gpioPulldownCount >= MAX_GPIO_PULL_PINS)
        {
            PRINT("Error: max %d pulldown pins already configured\n", MAX_GPIO_PULL_PINS);
            return;
        }
        gpioPulldownPins[gpioPulldownCount++] = (uint8_t)pin;
    }
    else
    {
        PRINT("Unknown function '%s' — use: irrx, irtx, pullup, pulldown\n", func);
        return;
    }

    saveToNvs();
    PRINT("GPIO %d → %s (reboot to apply)\n", pin, func);
}
