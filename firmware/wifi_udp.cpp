#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include "config.h"
#include "led.h"
#include "device.h"
#include "rmt_ir.h"
#include "wifi_udp.h"

WiFiUDP udp;
static uint8_t packetBuffer[IR_HEADER_SIZE + MAX_TIMING_VALUES * 2];

// ---- WiFi Setup ----
void setupWifi()
{
    Preferences prefs;
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
}

// ---- UDP Packet Dispatcher ----
void handleUdpPacket(uint8_t* data, int length)
{
    if (length < 2) return;
    uint16_t cmd;
    memcpy(&cmd, data, 2);
    switch (cmd)
    {
        case 1:  handleIrPacket(data, length);      break;
        default: LOG("UDP: unknown command %d\n", cmd);
    }
}

// ---- UDP Poll (called from loop) ----
void pollUdp()
{
    if (WiFi.status() != WL_CONNECTED) return;
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
