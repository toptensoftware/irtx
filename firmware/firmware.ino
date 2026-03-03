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
#include "ir.h"
#include "wifi_udp.h"
#include "serial.h"
#include "ble.h";

void setup()
{
    ledColor(4, 2, 0); // orange = starting

    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== ESP32-C3 IR Remote ===");

    setupDeviceName();
    setupIr();
    setupWifi();
    setupBle();

    ledColor(0, 4, 0); // green = ready
}

void loop()
{
    pollSerial();
    pollWifi();
    pollBle();
}
