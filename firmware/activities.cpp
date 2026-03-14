#include <Arduino.h>
#include <LittleFS.h>
#include "config.h"
#include "activities_types.h"
#include "activities.h"

#define BPAK_SIGNATURE  0x4B415042u  // 'BPAK'
#define BPAK_HEADER_SIZE 32

static uint8_t* activitiesData   = nullptr;
activitiesRoot* activitiesConfig = nullptr;

static bool loadActivities()
{
    File f = LittleFS.open("/activities.bin", "r");
    if (!f)
    {
        LOG("Activities: no activities.bin found\n");
        return false;
    }

    size_t size = f.size();
    if (size < BPAK_HEADER_SIZE)
    {
        LOG("Activities: file too small (%d bytes)\n", (int)size);
        f.close();
        return false;
    }

    uint8_t *data = (uint8_t *)malloc(size);
    if (!data)
    {
        LOG("Activities: failed to allocate %d bytes\n", (int)size);
        f.close();
        return false;
    }

    f.readBytes((char *)data, size);
    f.close();

    // Validate signature
    uint32_t *hdr = (uint32_t *)data;
    if (hdr[0] != BPAK_SIGNATURE)
    {
        LOG("Activities: bad signature 0x%08X\n", (unsigned)hdr[0]);
        free(data);
        return false;
    }

    // Parse header
    uint32_t relocCount  = hdr[2];
    uint32_t relocOffset = hdr[3];

    // Apply relocations — each entry is a file-relative offset to a pointer
    // field whose value is also file-relative; add the RAM base to each.
    uint32_t base = (uint32_t)data;
    uint32_t *relocs = (uint32_t *)(data + relocOffset);
    for (uint32_t i = 0; i < relocCount; i++)
    {
        uint32_t *ptrField = (uint32_t *)(data + relocs[i]);
        *ptrField += base;
    }

    activitiesData   = data;
    activitiesConfig = (activitiesRoot *)(data + BPAK_HEADER_SIZE);

    LOG("Activities: %d devices, %d activities\n",
        (int)activitiesConfig->devices_count, 
        (int)activitiesConfig->activities_count
    );
    return true;
}

void setupActivities()
{
    loadActivities();
}

bool reloadActivities()
{
    free(activitiesData);
    activitiesData = nullptr;
    activitiesConfig = nullptr;
    return loadActivities();
}

void pollActivities()
{
}
