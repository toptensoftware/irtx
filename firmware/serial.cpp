#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "led.h"
#include "device.h"
#include "wifi_udp.h"
#include "serial.h"
#include "ble.h"

static char inputLine[INPUT_MAX];
static int  inputLen  = 0;
static bool lastWasCR = false;

static void handleCommand(const char* line)
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
        ledColor(0, 4, 0);

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
    else if (strcmp(line, "status") == 0)
    {
        statusDeviceName();
        statusWifi();
        statusBle();
    }
    else if (*line == '\0')
    {
        // ignore empty line
    }
    else
    {
        LOG("Unknown command: %s\n", line);
        LOG("Commands: setwifi <ssid> <password> | name <devicename> | pair <0-3> | unpair <0-3> | status\n");
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
