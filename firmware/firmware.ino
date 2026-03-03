/**
 * ESP32-C3 IR Transmitter (WaveShare ESP32-C3 Zero)
 *
 * Receives commands over WiFi UDP and either:
 *   - Transmits IR timing sequences via IR LED (RMT peripheral)
 *
 * Hardware:
 *   - IR LED on GPIO 4
 *   - Onboard WS2812 RGB LED on GPIO 10 used for status
 */

#include <WiFi.h>
#include "config.h"
#include "led.h"
#include "device.h"
#include "rmt_ir.h"
#include "wifi_udp.h"
#include "serial_cmd.h"

void setup()
{
    ledColor(4, 2, 0); // orange = starting

    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== ESP32-C3 IR Remote ===");

    loadDeviceName();
    LOG("Device name: %s\n", deviceName);

    setupRmt();
    setupWifi();

    if (WiFi.status() == WL_CONNECTED)
    {
        udp.begin(UDP_PORT);
        LOG("Listening for UDP on port %d\n", UDP_PORT);
    }
    LOG("IR pin: GPIO %d, Carrier: %d Hz\n", IR_TX_PIN, CARRIER_FREQ);

    ledColor(0, 4, 0); // green = ready
}

void loop()
{
    handleSerialInput();

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

    pollUdp();
}
