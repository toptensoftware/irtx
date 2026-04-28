#include <Arduino.h>
#include "config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"

Preferences prefs;


void nvsDump() 
{
    nvs_iterator_t it = NULL;
    esp_err_t res = nvs_entry_find("nvs", nullptr, NVS_TYPE_ANY, &it);
    
    while (res == ESP_OK) 
    {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        
        PRINT("Namespace: %-16s | Key: %-16s | Type: %d\n",
               info.namespace_name, info.key, info.type);
        
        res = nvs_entry_next(&it);
    }
    nvs_release_iterator(it);
}


void nvsReset()
{
    PRINT("Erasing NVS...\n");
    nvs_flash_erase();
    nvs_flash_init();
    PRINT("NVS cleared\n");
}