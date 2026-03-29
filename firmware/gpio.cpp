#include <Arduino.h>
#include "config.h"
#include "gpio.h"
#include "activities.h"

int         gpioIrTxPin    = -1;
int         gpioIrRxPin    = -1;
int         gpioLedPin     = -1;
int         gpioLedOrder   = GPIO_LED_ORDER_GRB;
GpioPinSlot gpioPullupSlots[MAX_GPIO_PULL_PINS]   = {};
uint8_t     gpioPullupCount  = 0;
GpioPinSlot gpioPulldownSlots[MAX_GPIO_PULL_PINS] = {};
uint8_t     gpioPulldownCount = 0;


static void onButton(int pin, bool pressed)
{
    VERBOSE("Button %d %s\n", pin, pressed ? "pressed" : "released");
    invokeGpioBindings(pin, pressed);
}

static void onEncoder(int pin, int direction, uint32_t velocity)
{
    VERBOSE("Encoder %d dir=%+d velocity=%ums\n", pin, direction, velocity);
    invokeEncoderBindings(pin, direction, velocity);
}

// Remove any slot containing pin from both pullup and pulldown lists.
// Also clears irtx/irrx if they match.
static void removePinFromAllRoles(int pin)
{
    if (gpioIrTxPin == pin) gpioIrTxPin = -1;
    if (gpioIrRxPin == pin) gpioIrRxPin = -1;
    if (gpioLedPin  == pin) gpioLedPin  = -1;

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
    prefs.putInt("irtx",     gpioIrTxPin);
    prefs.putInt("irrx",     gpioIrRxPin);
    prefs.putInt("led",      gpioLedPin);
    prefs.putInt("ledorder", gpioLedOrder);
    prefs.putBytes("pullup",   gpioPullupSlots,   gpioPullupCount   * sizeof(GpioPinSlot));
    prefs.putBytes("pulldown", gpioPulldownSlots, gpioPulldownCount * sizeof(GpioPinSlot));
    prefs.end();
}

static void initPollState();  // forward declaration — defined below

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
    gpioIrTxPin  = prefs.getInt("irtx",     -1);
    gpioIrRxPin  = prefs.getInt("irrx",     -1);
    gpioLedPin   = prefs.getInt("led",      -1);
    gpioLedOrder = prefs.getInt("ledorder", GPIO_LED_ORDER_GRB);
    size_t upBytes   = prefs.getBytes("pullup",   gpioPullupSlots,
                                      MAX_GPIO_PULL_PINS * sizeof(GpioPinSlot));
    size_t downBytes = prefs.getBytes("pulldown", gpioPulldownSlots,
                                      MAX_GPIO_PULL_PINS * sizeof(GpioPinSlot));
    prefs.end();

    gpioPullupCount   = (uint8_t)(upBytes   / sizeof(GpioPinSlot));
    gpioPulldownCount = (uint8_t)(downBytes / sizeof(GpioPinSlot));

    applyPullModes(gpioPullupSlots,   gpioPullupCount,   INPUT_PULLUP);
    applyPullModes(gpioPulldownSlots, gpioPulldownCount, INPUT_PULLDOWN);
    initPollState();

    LOG("GPIO config: irtx=%d irrx=%d led=%d pullup=%d pulldown=%d\n",
        gpioIrTxPin, gpioIrRxPin, gpioLedPin, gpioPullupCount, gpioPulldownCount);
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
    PRINT("LED pin:   %d (%s)\n", gpioLedPin,
          gpioLedPin < 0 ? "disabled" :
          gpioLedOrder == GPIO_LED_ORDER_GRB ? "grb" : "rgb");

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

    if (strcmp(func, "grb") == 0)
    {
        gpioLedPin   = pinA;
        gpioLedOrder = GPIO_LED_ORDER_GRB;
    }
    else if (strcmp(func, "rgb") == 0)
    {
        gpioLedPin   = pinA;
        gpioLedOrder = GPIO_LED_ORDER_RGB;
    }
    else if (strcmp(func, "irtx") == 0)
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
        PRINT("Unknown function '%s' — use: grb, rgb, irrx, irtx, pullup, pulldown\n", func);
        return;
    }

    saveToNvs();

    if (pinB >= 0)
        PRINT("GPIO %d+%d → %s (reboot to apply)\n", pinA, pinB, func);
    else
        PRINT("GPIO %d → %s (reboot to apply)\n", pinA, func);
}

// ─── Poll state ────────────────────────────────────────────────────────────────

#define BUTTON_DEBOUNCE_MS  10
#define ENCODER_DEBOUNCE_MS  2  // min ms between accepted encoder transitions

static struct ButtonState {
    bool     raw;           // last raw digitalRead
    bool     stable;        // debounced state (true = HIGH)
    uint32_t rawChangeMs;   // when raw last changed
} s_btnUp[MAX_GPIO_PULL_PINS], s_btnDown[MAX_GPIO_PULL_PINS];

// Quadrature grey-code lookup: index = (prevAB << 2) | currAB
// Returns +1 (CW), -1 (CCW), or 0 (no change / invalid)
static const int8_t s_encTable[16] = {
//  curr→  00   01   10   11
           0,   1,  -1,   0,   // prev 00
          -1,   0,   0,   1,   // prev 01
           1,   0,   0,  -1,   // prev 10
           0,  -1,   1,   0,   // prev 11
};

static struct EncoderState {
    uint8_t  prevAB;            // last A+B reading
    int8_t   accum;             // signed tick accumulator; fire at ±4
    uint32_t tickTimes[4];      // circular buffer of last 4 tick timestamps (ms)
    uint8_t  tickHead;          // next write position in tickTimes
    uint32_t lastTransitionMs;  // time of last accepted transition (for debounce)
} s_encUp[MAX_GPIO_PULL_PINS], s_encDown[MAX_GPIO_PULL_PINS];

static void initPollSlotState(GpioPinSlot* slots, uint8_t count,
                               ButtonState* btn, EncoderState* enc)
{
    uint32_t now = millis();
    for (int i = 0; i < count; i++)
    {
        if (slots[i].pinB == GPIO_PIN_NONE)
        {
            bool raw = (bool)digitalRead(slots[i].pinA);
            btn[i].raw        = raw;
            btn[i].stable     = raw;
            btn[i].rawChangeMs = now;
        }
        else
        {
            uint8_t a = (uint8_t)digitalRead(slots[i].pinA);
            uint8_t b = (uint8_t)digitalRead(slots[i].pinB);
            enc[i].prevAB           = (a << 1) | b;
            enc[i].accum            = 0;
            enc[i].tickHead         = 0;
            enc[i].lastTransitionMs = now;
            for (int j = 0; j < 4; j++) enc[i].tickTimes[j] = now;
        }
    }
}

static void initPollState()
{
    initPollSlotState(gpioPullupSlots,   gpioPullupCount,   s_btnUp,   s_encUp);
    initPollSlotState(gpioPulldownSlots, gpioPulldownCount, s_btnDown, s_encDown);
}

static void pollButtonSlot(int pin, bool pullup, ButtonState& s)
{
    bool raw = (bool)digitalRead(pin);
    uint32_t now = millis();

    if (raw != s.raw)
    {
        s.raw = raw;
        s.rawChangeMs = now;
    }
    if (s.raw != s.stable && (now - s.rawChangeMs) >= BUTTON_DEBOUNCE_MS)
    {
        s.stable = s.raw;
        bool pressed = pullup ? !s.stable : s.stable;
        onButton(pin, pressed);
    }
}

static void pollEncoderSlot(int pinA, int pinB, EncoderState& s)
{
    uint8_t a  = (uint8_t)digitalRead(pinA);
    uint8_t b  = (uint8_t)digitalRead(pinB);
    uint8_t ab = (a << 1) | b;

    if (ab == s.prevAB) return;

    int8_t dir = s_encTable[(s.prevAB << 2) | ab];
    s.prevAB = ab;  // always track current state regardless of debounce or validity

    if (dir == 0) return;   // invalid grey-code transition (noise), ignore

    uint32_t now = millis();
    if (now - s.lastTransitionMs < ENCODER_DEBOUNCE_MS) return;
    s.lastTransitionMs = now;
    s.tickTimes[s.tickHead] = now;
    s.tickHead = (s.tickHead + 1) % 4;
    s.accum += dir;

    if (s.accum >= 4 || s.accum <= -4)
    {
        int direction     = s.accum > 0 ? 1 : -1;
        uint32_t oldest   = s.tickTimes[s.tickHead]; // tickHead now points to oldest
        uint32_t velocity = now - oldest;
        s.accum -= direction * 4;
        onEncoder(pinA, direction, velocity);
    }
}

static void pollPullSlots(GpioPinSlot* slots, uint8_t count, bool pullup,
                           ButtonState* btn, EncoderState* enc)
{
    for (int i = 0; i < count; i++)
    {
        if (slots[i].pinB == GPIO_PIN_NONE)
            pollButtonSlot(slots[i].pinA, pullup, btn[i]);
        else
            pollEncoderSlot(slots[i].pinA, slots[i].pinB, enc[i]);
    }
}

void pollGpio()
{
    pollPullSlots(gpioPullupSlots,   gpioPullupCount,   true,  s_btnUp,   s_encUp);
    pollPullSlots(gpioPulldownSlots, gpioPulldownCount, false, s_btnDown, s_encDown);
}

