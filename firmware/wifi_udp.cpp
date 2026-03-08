#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "esp_mac.h"
#include "config.h"
#include "led.h"
#include "device.h"
#include "ir_tx.h"
#include "wifi_udp.h"
#include "ir_router.h"
#include "ble.h"

WiFiUDP udp;
static uint8_t packetBuffer[IR_HEADER_SIZE + MAX_TIMING_VALUES * 2];

// ---- WiFi Setup ----
void setupWifi()
{
    prefs.begin("wifi", true);
    String wifiSsid = prefs.getString("ssid", "");
    String wifiPass = prefs.getString("password", "");
    prefs.end();

    if (wifiSsid.isEmpty())
    {
        LOG("No WiFi configured. Use: setwifi <ssid> <password>\n");
        return;
    }

    LOG("Connecting to %s\n", wifiSsid.c_str());
    WiFi.setHostname(deviceName);  // must be before WiFi.mode() in arduino-esp32 v3.x
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);          // disable modem sleep to reduce WiFi/BLE radio contention
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());

    bool toggle = false;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        toggle = !toggle;
        ledColor(0, 0, toggle ? 4 : 0);
    }
    LOG("Connected! IP: %s\n", WiFi.localIP().toString().c_str());

    if (WiFi.status() == WL_CONNECTED)
    {
        udp.begin(UDP_PORT);
        LOG("Listening for UDP on port %d\n", UDP_PORT);
    }

}

// ---- UDP Packet Dispatcher ----
void handleUdpPacket(uint8_t* data, int length)
{
    if (length < 2) return;
    uint16_t cmd;
    memcpy(&cmd, data, 2);
    switch (cmd)
    {
        case 1: handleIrPacket(data, length);      break;
        case 2: handleBleConnectPacket(data, length);     break;
        case 3: handleBleHidPacket(data, length);     break;
        case 4: handleIrCodePacket(data, length);      break;
        case 5: handleRoutePacket(data, length);       break;
        default: LOG("UDP: unknown command %d\n", cmd);
    }
}

// ---- UDP Poll (called from loop) ----
void pollWifi()
{
    // WiFi reconnect
    if (WiFi.getMode() != WIFI_OFF && WiFi.status() != WL_CONNECTED)
    {
        ledColor(4, 2, 0);
        LOG("WiFi disconnected, reconnecting...\n");
        WiFi.reconnect();
        while (WiFi.status() != WL_CONNECTED) delay(500);
        LOG("Reconnected! IP: %s\n", WiFi.localIP().toString().c_str());
        ledColor(0, 4, 0);
    }


    if (WiFi.status() != WL_CONNECTED) 
        return;
        
    int packetSize = udp.parsePacket();
    if (packetSize >= 2)
    {
        int bytesRead = udp.read(packetBuffer, min((int)sizeof(packetBuffer), packetSize));
        handleUdpPacket(packetBuffer, bytesRead);
    }
    else if (packetSize > 0)
    {
        udp.read(packetBuffer, packetSize); // flush runt
    }
}


void statusWifi()
{
    PRINT("--- WiFi ---\n");
    prefs.begin("wifi", true);
    String ssid = prefs.getString("ssid", "(not set)");
    prefs.end();
    PRINT("SSID        : %s\n", ssid.c_str());
    if (WiFi.status() == WL_CONNECTED)
    {
        PRINT("IP address  : %s\n", WiFi.localIP().toString().c_str());
        PRINT("Gateway     : %s\n", WiFi.gatewayIP().toString().c_str());
        PRINT("RSSI        : %d dBm\n", WiFi.RSSI());
    }
    else
    {
        PRINT("Status      : disconnected\n");
    }
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    PRINT("MAC (WiFi)  : %02X:%02X:%02X:%02X:%02X:%02X\n",
                    mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
    PRINT("UDP port    : %d\n", UDP_PORT);
}