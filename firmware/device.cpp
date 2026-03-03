#include <Arduino.h>
#include <Preferences.h>
#include "device.h"

char deviceName[64] = "urem";

void loadDeviceName()
{
    Preferences prefs;
    prefs.begin("device", true);
    String n = prefs.getString("name", "urem");
    prefs.end();
    n.trim();
    if (n.length() > 0 && n.length() < sizeof(deviceName))
        strncpy(deviceName, n.c_str(), sizeof(deviceName) - 1);
}
