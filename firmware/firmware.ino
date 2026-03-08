/**
 * ESP32 IR Transmitter (WaveShare ESP32-C3 Zero / ESP32-C6)
 *
 * Receives commands over WiFi UDP and either:
 *   - Transmits IR timing sequences via IR LED (RMT peripheral)
 *
 * Hardware:
 *   - IR LED on GPIO 4 (C3) / GPIO 11 (C6)
 *   - Onboard WS2812 RGB LED on GPIO 10 (C3) / GPIO 8 (C6)
 */

#include <WiFi.h>
#include "config.h"
#include "led.h"
#include "device.h"
#include "ir_rx.h"
#include "ir_tx.h"
#include "wifi_udp.h"
#include "serial.h"
#include "ble.h";

void setup()
{
    ledColor(4, 2, 0); // orange = starting

    Serial.begin(115200);
    delay(2000);
    Serial.println("\n=== ESP32 IR Remote ===");

    setupDeviceName();
    setupIrTx();
    setupIrRx();
    setupWifi();
    setupBle();

    ledColor(0, 4, 0); // green = ready
}

void loop()
{
    pollSerial();
    pollIrRx();
    pollWifi();
    pollBle();
}
