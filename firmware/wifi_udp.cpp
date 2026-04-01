#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "esp_mac.h"
#include "config.h"
#include "led.h"
#include "device.h"
#include "ir_tx.h"
#include "wifi_udp.h"
#include "ble.h"
#include "activities.h"
#include "ir_rx.h"
#include "gpio.h"

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
        setLed(LED_PRIORITY_CONNECTIVITY, toggle ? 0x000004 : 0);
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
        case 1:
            if (isActivitiesBusy()) { LOG("UDP: IR packet dropped (activities busy)\n"); break; }
            handleIrPacket(data, length);
            break;
        case 2: handleBleConnectPacket(data, length);     break;
        case 3: handleBleHidPacket(data, length);     break;
        case 4:
            if (isActivitiesBusy()) { LOG("UDP: IR code packet dropped (activities busy)\n"); break; }
            handleIrCodePacket(data, length);
            break;
        case 5:
        {
            // [uint16 cmd=5][uint32 activityIndex]
            if (length < 6) { LOG("UDP: activity packet too short\n"); break; }
            uint32_t index;
            memcpy(&index, data + 2, 4);
            switchActivity((int)index);
            break;
        }
        case 6:
        {
            // [uint16 cmd=6][uint32 protocol][uint64 code][uint32 eventKindMask]
            if (length < 18) { LOG("UDP: simulate IR packet too short\n"); break; }
            uint32_t protocol;
            uint64_t code;
            uint32_t eventKindMask;
            memcpy(&protocol,     data + 2,  4);
            memcpy(&code,         data + 6,  8);
            memcpy(&eventKindMask, data + 14, 4);
            simulateIrRx(protocol, code, eventKindMask);
            break;
        }
        default: LOG("UDP: unknown command %d\n", cmd);
    }
}

// ---- UDP Poll (called from loop) ----
void pollWifi()
{
    if (WiFi.getMode() != WIFI_AP)
    {
        // STA mode: handle reconnect
        if (WiFi.getMode() != WIFI_OFF && WiFi.status() != WL_CONNECTED)
        {
            setLed(LED_PRIORITY_CONNECTIVITY, 0x040200);
            LOG("WiFi disconnected, reconnecting...\n");
            WiFi.reconnect();
            while (WiFi.status() != WL_CONNECTED) delay(500);
            LOG("Reconnected! IP: %s\n", WiFi.localIP().toString().c_str());
            setLed(LED_PRIORITY_CONNECTIVITY, 0x000400);
        }

        if (WiFi.status() != WL_CONNECTED)
            return;
    }

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

// ---- Boot-pin AP check ----

static bool isBootPinPressed(int pin)
{
    // Pullup: pin is active-low, pressed = LOW
    for (int i = 0; i < gpioPullupCount; i++)
        if (gpioPullupSlots[i].pinA == (uint8_t)pin || gpioPullupSlots[i].pinB == (uint8_t)pin)
            return digitalRead(pin) == LOW;
    // Pulldown: pin is active-high, pressed = HIGH
    for (int i = 0; i < gpioPulldownCount; i++)
        if (gpioPulldownSlots[i].pinA == (uint8_t)pin || gpioPulldownSlots[i].pinB == (uint8_t)pin)
            return digitalRead(pin) == HIGH;
    // Not found in either list — assume pullup behaviour
    return digitalRead(pin) == LOW;
}

void setupWifiOrAP()
{
    prefs.begin("device", true);
    int pin1 = prefs.getInt("bootpin1", -1);
    int pin2 = prefs.getInt("bootpin2", -1);
    prefs.end();

    if (pin1 >= 0)
    {
        prefs.begin("ap", true);
        String apSsid = prefs.getString("ssid", "");
        prefs.end();

        if (!apSsid.isEmpty() && isBootPinPressed(pin1) && (pin2 < 0 || isBootPinPressed(pin2)))
        {
            LOG("Boot pin pressed — starting access point\n");
            startAccessPoint();
            return;
        }
    }

    setupWifi();
}

// ---- Switch to Access Point mode ----
void startAccessPoint()
{
    prefs.begin("ap", true);
    String apSsid = prefs.getString("ssid", deviceName);
    String apPass = prefs.getString("password", "irtx");
    prefs.end();

    LOG("Switching to access point mode: %s\n", apSsid.c_str());
    udp.stop();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid.c_str(), apPass.c_str());
    udp.begin(UDP_PORT);
    LOG("Access point started. IP: %s\n", WiFi.softAPIP().toString().c_str());
}


void statusWifi()
{
    PRINT("--- WiFi ---\n");
    if (WiFi.getMode() == WIFI_AP)
    {
        prefs.begin("ap", true);
        String apSsid = prefs.getString("ssid", deviceName);
        prefs.end();
        PRINT("Mode        : AP\n");
        PRINT("AP SSID     : %s\n", apSsid.c_str());
        PRINT("AP IP       : %s\n", WiFi.softAPIP().toString().c_str());
    }
    else
    {
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
    }
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    PRINT("MAC (WiFi)  : %02X:%02X:%02X:%02X:%02X:%02X\n",
                    mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
    PRINT("UDP port    : %d\n", UDP_PORT);
}