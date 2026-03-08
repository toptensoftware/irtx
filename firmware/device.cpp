#include <Arduino.h>
#include "device.h"
#include "config.h"

char deviceName[64] = "urem";

void setupDeviceName()
{
    prefs.begin("device", true);
    String n = prefs.getString("name", "urem");
    prefs.end();
    n.trim();
    if (n.length() > 0 && n.length() < sizeof(deviceName))
        strncpy(deviceName, n.c_str(), sizeof(deviceName) - 1);

    LOG("Device name \"%s\"\n", deviceName);
}


void statusDeviceName()
{
    Serial.printf("--- Device ---\n");
    Serial.printf("Name        : %s\n", deviceName);
}