#include <Arduino.h>
#include <LittleFS.h>
#include "config.h"
#include "filesystem.h"

void setupFs()
{
    if (!LittleFS.begin(false))
    {
        LOG("LittleFS mount failed, formatting...\n");
        if (!LittleFS.begin(true))
        {
            LOG("LittleFS format failed\n");
            return;
        }
    }

    LOG("LittleFS mounted - %d KB used / %d KB total\n",
        (int)(LittleFS.usedBytes() / 1024),
        (int)(LittleFS.totalBytes() / 1024));
}
