#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "led.h"
#include "device.h"
#include "wifi_udp.h"
#include "serial.h"
#include "ble.h"
#include "log.h"
#include "esp_system.h"
#include "ir_protocol.h"
#include "activities.h"
#include "gpio.h"

static char inputLine[INPUT_MAX];
static int  inputLen  = 0;
static bool lastWasCR = false;

void handleCommand(const char* line)
{
    while (*line == ' ') line++;

    if (strncmp(line, "setwifi ", 8) == 0)
    {
        const char* p = line + 8;
        while (*p == ' ') p++;
        const char* ssidStart = p;
        while (*p && *p != ' ') p++;
        int ssidLen = p - ssidStart;
        if (ssidLen == 0) { LOG("Usage: setwifi <ssid> <password>\n"); return; }
        char ssid[128];
        ssidLen = min(ssidLen, 127);
        memcpy(ssid, ssidStart, ssidLen); ssid[ssidLen] = '\0';
        while (*p == ' ') p++;
        int passLen = strlen(p);
        while (passLen > 0 && p[passLen-1] == ' ') passLen--;
        char password[256];
        passLen = min(passLen, 255);
        memcpy(password, p, passLen); password[passLen] = '\0';
        prefs.begin("wifi", false);
        prefs.putString("ssid", ssid);
        prefs.putString("password", password);
        prefs.end();
        LOG("Saved WiFi: ssid=\"%s\"\n", ssid);
        LOG("Reconnecting...\n");
        WiFi.disconnect();
        setupWifi();
        if (WiFi.status() == WL_CONNECTED)
        {
            udp.begin(UDP_PORT);
            LOG("Listening for UDP on port %d\n", UDP_PORT);
        }

    }
    else if (strncmp(line, "setap ", 6) == 0)
    {
        const char* p = line + 6;
        while (*p == ' ') p++;
        const char* ssidStart = p;
        while (*p && *p != ' ') p++;
        int ssidLen = p - ssidStart;
        if (ssidLen == 0) { LOG("Usage: setap <ssid> [<password>]\n"); return; }
        char ssid[128];
        ssidLen = min(ssidLen, 127);
        memcpy(ssid, ssidStart, ssidLen); ssid[ssidLen] = '\0';
        while (*p == ' ') p++;
        int passLen = strlen(p);
        while (passLen > 0 && p[passLen-1] == ' ') passLen--;
        char password[256];
        if (passLen == 0)
        {
            strcpy(password, "irtx1234");
        }
        else
        {
            if (passLen < 8) { LOG("Error: AP password must be at least 8 characters\n"); return; }
            passLen = min(passLen, 255);
            memcpy(password, p, passLen); password[passLen] = '\0';
        }
        prefs.begin("ap", false);
        prefs.putString("ssid", ssid);
        prefs.putString("password", password);
        prefs.end();
        LOG("Saved AP: ssid=\"%s\"\n", ssid);

    }
    else if (strncmp(line, "setbootpin ", 11) == 0)
    {
        const char* p = line + 11;
        while (*p == ' ') p++;
        char* end;
        long pin1 = strtol(p, &end, 10);
        if (end == p) { LOG("Usage: setbootpin <pin> [<pin>]\n"); return; }
        p = end;
        while (*p == ' ') p++;
        long pin2 = -1;
        if (*p)
        {
            char* end2;
            long v = strtol(p, &end2, 10);
            if (end2 != p) pin2 = v;
        }
        prefs.begin("device", false);
        prefs.putInt("bootpin1", (int)pin1);
        prefs.putInt("bootpin2", (int)pin2);
        prefs.end();
        if (pin2 >= 0)
            LOG("Boot AP pins set to %ld+%ld\n", pin1, pin2);
        else
            LOG("Boot AP pin set to %ld\n", pin1);

    }
    else if (strncmp(line, "setdefact ", 10) == 0)
    {
        const char* p = line + 10;
        while (*p == ' ') p++;
        char* end;
        long idx = strtol(p, &end, 10);
        if (end == p || idx < 0) { LOG("Usage: setdefact <index>\n"); return; }
        prefs.begin("device", false);
        prefs.putInt("defact", (int)idx);
        prefs.end();
        LOG("Default activity set to %ld\n", idx);

    }
    else if (strncmp(line, "name ", 5) == 0)
    {
        const char* p = line + 5;
        while (*p == ' ') p++;
        int nameLen = strlen(p);
        while (nameLen > 0 && p[nameLen-1] == ' ') nameLen--;
        if (nameLen == 0 || nameLen >= (int)sizeof(deviceName))
        {
            LOG("Usage: name <devicename> (1-%d chars)\n", (int)sizeof(deviceName) - 1);
            return;
        }
        char newName[64];
        memcpy(newName, p, nameLen); newName[nameLen] = '\0';
        prefs.begin("device", false);
        prefs.putString("name", newName);
        prefs.end();
        LOG("Device name set to \"%s\" — restart to apply\n", newName);

    }
    else if (strncmp(line, "pair ", 5) == 0) 
    {
        blePair(atoi(line + 5));
    } 
    else if (strncmp(line, "unpair ", 7) == 0) 
    {
        bleUnpair(atoi(line + 7));
    }
    else if (strncmp(line, "connect ", 8) == 0) 
    {
        bleConnect(atoi(line + 8));
    }
    else if (strncmp(line, "led ", 4) == 0)
    {
        int r, g, b;
        if (strcmp(line + 4, "clear") == 0)
            setLed(LED_PRIORITY_USER, 0xFFFFFFFF);
        else if (sscanf(line + 4, "%d %d %d", &r, &g, &b) == 3)
            setLed(LED_PRIORITY_USER, ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF));
        else
            LOG("Usage: led [ clear | <r> <g> <b> ]\n");
    }
    else if (strncmp(line, "activity ", 9) == 0)
    {
        const char* arg = line + 9;
        while (*arg == ' ') arg++;
        if (!activitiesConfig)
        {
            PRINT("No activities config loaded\n");
            return;
        }
        // Accept either an index or a name.
        int idx = -1;
        // Try name match first.
        for (uint32_t i = 0; i < activitiesConfig->activities_count; i++)
        {
            if (strcmp(activitiesConfig->activities[i].name, arg) == 0)
            {
                idx = (int)i;
                break;
            }
        }
        // Fall back to numeric index.
        if (idx < 0)
        {
            char* end;
            long n = strtol(arg, &end, 10);
            if (end != arg) idx = (int)n;
        }
        if (idx < 0)
            PRINT("Unknown activity: %s\n", arg);
        else
            switchActivity(idx);
    }
    else if (strncmp(line, "gpio", 4) == 0 && (line[4] == ' ' || line[4] == '\0'))
    {
        const char* p = line + 4;
        while (*p == ' ') p++;

        if (*p == '\0')
        {
            statusGpioConfig();
            return;
        }

        char* end;
        int pinA = (int)strtol(p, &end, 10);
        if (end == p)
        {
            PRINT("Usage: gpio [<pin> [<pin>] <grb|rgb|irrx|irtx|pullup|pulldown>]\n");
            return;
        }
        p = end;
        while (*p == ' ') p++;

        // Optional second pin (for encoder pairs)
        int pinB = -1;
        char* end2;
        long maybePin = strtol(p, &end2, 10);
        if (end2 != p && (*end2 == ' ' || *end2 == '\0'))
        {
            pinB = (int)maybePin;
            p = end2;
            while (*p == ' ') p++;
        }

        if (*p == '\0')
        {
            PRINT("Usage: gpio [<pin> [<pin>] <grb|rgb|irrx|irtx|pullup|pulldown>]\n");
            return;
        }

        gpioSetPin(pinA, pinB, p);
    }
    else if (strcmp(line, "status") == 0)
    {
        PRINT("{\n");
        statusDeviceName();
        statusGpioConfig();
        statusProtocols();
        statusWifi();
        statusBle();
        statusActivities();
        PRINT("}\n");
    }
    else if (strcmp(line, "dmesg") == 0)
    {
        dmesgPrint();
    }
    else if (strcmp(line, "nvsdump") == 0)
    {
        nvsDump();
    }
    else if (strcmp(line, "nvsreset") == 0)
    {
        nvsReset();
    }
    else if (strncmp(line, "verbose ", 8) == 0)
    {
        const char* arg = line + 8;
        while (*arg == ' ') arg++;
        if (strcmp(arg, "on") == 0)
        {
            logSetVerbose(true);
            prefs.begin("device", false);
            prefs.putBool("verbose", true);
            prefs.end();
            LOG("Verbose logging on\n");
        }
        else if (strcmp(arg, "off") == 0)
        {
            logSetVerbose(false);
            prefs.begin("device", false);
            prefs.putBool("verbose", false);
            prefs.end();
            LOG("Verbose logging off\n");
        }
        else
        {
            PRINT("Usage: verbose on|off\n");
        }
    }
    else if (strcmp(line, "reboot") == 0)
    {
        PRINT("Rebooting...\n");
        delay(500);
        esp_restart();
    }
    else if (strcmp(line, "help") == 0)
    {
        PRINT("Commands:\n");
        PRINT("  setwifi <ssid> <password>              Save WiFi credentials and reconnect\n");
        PRINT("  setap <ssid> [<password>]              Save AP credentials (pw must be 8+ chars, default: irtx1234)\n");
        PRINT("  setbootpin <pin> [<pin>]               Pin(s) that trigger AP mode at boot\n");
        PRINT("  name <devicename>                      Set device name (restart to apply)\n");
        PRINT("  activity <name|index>                  Switch to activity by name or index\n");
        PRINT("  setdefact <index>                      Set default activity loaded on boot\n");
        PRINT("  gpio [<pin> [<pin>] <mode>]            Show or set GPIO pin mode\n");
        PRINT("    modes: grb, rgb, irrx, irtx, pullup, pulldown\n");
        PRINT("  led <r> <g> <b>                        Set LED colour\n");
        PRINT("  led clear                              Clear user LED override\n");
        PRINT("  pair <index>                           Pair BLE device at index\n");
        PRINT("  unpair <index>                         Unpair BLE device at index\n");
        PRINT("  connect <index>                        Connect to paired BLE device\n");
        PRINT("  verbose on|off                         Enable or disable verbose logging\n");
        PRINT("  status                                 Show full device status\n");
        PRINT("  dmesg                                  Print boot log\n");
        PRINT("  nvsdump                                Dump NVS storage contents\n");
        PRINT("  nvsreset                               Reset all NVS storage\n");
        PRINT("  reboot                                 Restart the device\n");
        PRINT("  help                                   Show this help\n");
    }
    else if (*line == '\0')
    {
        // ignore empty line
    }
    else
    {
        PRINT("Unknown command: %s\n", line);
    }
}

void pollSerial()
{
    while (Serial.available())
    {
        char c = Serial.read();
        if (c == '\n' && lastWasCR)
        {
            lastWasCR = false;
            continue;
        }
        lastWasCR = (c == '\r');
        if (c == '\r' || c == '\n')
        {
            Serial.println();
            inputLine[inputLen] = '\0';
            handleCommand(inputLine);
            inputLen = 0;
        }
        else if (c == '\b' || c == 127)
        {
            if (inputLen > 0) { inputLen--; Serial.print("\b \b"); }
        }
        else if (c >= 32 && inputLen < INPUT_MAX - 1)
        {
            inputLine[inputLen++] = c;
            Serial.print(c);
        }
    }
}
