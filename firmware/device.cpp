#include <Arduino.h>
#include "device.h"
#include "config.h"
#include "log.h"

char deviceName[64] = "urem";

void setupDeviceName()
{
    prefs.begin("device", true);
    String n = prefs.getString("name", "urem");
    bool verbose = prefs.getBool("verbose", false);
    prefs.end();

    logSetVerbose(verbose);
    n.trim();
    if (n.length() > 0 && n.length() < sizeof(deviceName))
        strncpy(deviceName, n.c_str(), sizeof(deviceName) - 1);

    LOG("Device name \"%s\"\n", deviceName);
}


void statusDeviceName()
{
    PRINT("  \"device\": {\n");
    PRINT("    \"name\": "); printJsonString(deviceName); PRINT(",\n");
    PRINT("    \"logging\": \"%s\"\n", logGetVerbose() ? "verbose" : "standard");
    PRINT("  },\n");
}