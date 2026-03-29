#include <Arduino.h>
#include "config.h"
#include "gpio_config.h"

int         gpioIrTxPin    = -1;
int         gpioIrRxPin    = -1;
GpioPinSlot gpioPullupSlots[MAX_GPIO_PULL_PINS]   = {};
uint8_t     gpioPullupCount  = 0;
GpioPinSlot gpioPulldownSlots[MAX_GPIO_PULL_PINS] = {};
uint8_t     gpioPulldownCount = 0;

// Remove any slot containing pin from both pullup and pulldown lists.
// Also clears irtx/irrx if they match.
static void removePinFromAllRoles(int pin)
{
    if (gpioIrTxPin == pin) gpioIrTxPin = -1;
    if (gpioIrRxPin == pin) gpioIrRxPin = -1;

    for (int i = 0; i < gpioPullupCount; i++)
    {
        if (gpioPullupSlots[i].pinA == (uint8_t)pin ||
            gpioPullupSlots[i].pinB == (uint8_t)pin)
        {
            memmove(&gpioPullupSlots[i], &gpioPullupSlots[i + 1],
                    (gpioPullupCount - i - 1) * sizeof(GpioPinSlot));
            gpioPullupCount--;
            break;
        }
    }
    for (int i = 0; i < gpioPulldownCount; i++)
    {
        if (gpioPulldownSlots[i].pinA == (uint8_t)pin ||
            gpioPulldownSlots[i].pinB == (uint8_t)pin)
        {
            memmove(&gpioPulldownSlots[i], &gpioPulldownSlots[i + 1],
                    (gpioPulldownCount - i - 1) * sizeof(GpioPinSlot));
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
    prefs.putBytes("pullup",   gpioPullupSlots,   gpioPullupCount   * sizeof(GpioPinSlot));
    prefs.putBytes("pulldown", gpioPulldownSlots, gpioPulldownCount * sizeof(GpioPinSlot));
    prefs.end();
}

static void applyPullModes(GpioPinSlot* slots, uint8_t count, int mode)
{
    for (int i = 0; i < count; i++)
    {
        pinMode(slots[i].pinA, mode);
        if (slots[i].pinB != GPIO_PIN_NONE)
            pinMode(slots[i].pinB, mode);
    }
}

void setupGpioConfig()
{
    extern Preferences prefs;
    prefs.begin("gpio", true);
    gpioIrTxPin = prefs.getInt("irtx", -1);
    gpioIrRxPin = prefs.getInt("irrx", -1);
    size_t upBytes   = prefs.getBytes("pullup",   gpioPullupSlots,
                                      MAX_GPIO_PULL_PINS * sizeof(GpioPinSlot));
    size_t downBytes = prefs.getBytes("pulldown", gpioPulldownSlots,
                                      MAX_GPIO_PULL_PINS * sizeof(GpioPinSlot));
    prefs.end();

    gpioPullupCount   = (uint8_t)(upBytes   / sizeof(GpioPinSlot));
    gpioPulldownCount = (uint8_t)(downBytes / sizeof(GpioPinSlot));

    applyPullModes(gpioPullupSlots,   gpioPullupCount,   INPUT_PULLUP);
    applyPullModes(gpioPulldownSlots, gpioPulldownCount, INPUT_PULLDOWN);

    LOG("GPIO config: irtx=%d irrx=%d pullup=%d pulldown=%d\n",
        gpioIrTxPin, gpioIrRxPin, gpioPullupCount, gpioPulldownCount);
}

static void printSlots(GpioPinSlot* slots, uint8_t count)
{
    if (count == 0) { PRINT(" (none)"); return; }
    for (int i = 0; i < count; i++)
    {
        if (slots[i].pinB == GPIO_PIN_NONE)
            PRINT(" %d", slots[i].pinA);
        else
            PRINT(" %d+%d", slots[i].pinA, slots[i].pinB);
    }
}

void statusGpioConfig()
{
    PRINT("IR TX pin: %d\n", gpioIrTxPin);
    PRINT("IR RX pin: %d\n", gpioIrRxPin);

    PRINT("Pullup slots (%d):", gpioPullupCount);
    printSlots(gpioPullupSlots, gpioPullupCount);
    PRINT("\n");

    PRINT("Pulldown slots (%d):", gpioPulldownCount);
    printSlots(gpioPulldownSlots, gpioPulldownCount);
    PRINT("\n");
}

void gpioSetPin(int pinA, int pinB, const char* func)
{
    if (pinA < 0)
    {
        PRINT("Invalid pin: %d\n", pinA);
        return;
    }

    removePinFromAllRoles(pinA);
    if (pinB >= 0)
        removePinFromAllRoles(pinB);

    if (strcmp(func, "irtx") == 0)
    {
        gpioIrTxPin = pinA;
    }
    else if (strcmp(func, "irrx") == 0)
    {
        gpioIrRxPin = pinA;
    }
    else if (strcmp(func, "pullup") == 0)
    {
        if (gpioPullupCount >= MAX_GPIO_PULL_PINS)
        {
            PRINT("Error: max %d pullup slots already configured\n", MAX_GPIO_PULL_PINS);
            return;
        }
        gpioPullupSlots[gpioPullupCount++] = { (uint8_t)pinA,
                                               pinB >= 0 ? (uint8_t)pinB : GPIO_PIN_NONE };
    }
    else if (strcmp(func, "pulldown") == 0)
    {
        if (gpioPulldownCount >= MAX_GPIO_PULL_PINS)
        {
            PRINT("Error: max %d pulldown slots already configured\n", MAX_GPIO_PULL_PINS);
            return;
        }
        gpioPulldownSlots[gpioPulldownCount++] = { (uint8_t)pinA,
                                                   pinB >= 0 ? (uint8_t)pinB : GPIO_PIN_NONE };
    }
    else
    {
        PRINT("Unknown function '%s' — use: irrx, irtx, pullup, pulldown\n", func);
        return;
    }

    saveToNvs();

    if (pinB >= 0)
        PRINT("GPIO %d+%d → %s (reboot to apply)\n", pinA, pinB, func);
    else
        PRINT("GPIO %d → %s (reboot to apply)\n", pinA, func);
}
