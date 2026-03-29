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
#include "telnet.h"
#include "ble.h"
#include "filesystem.h"
#include "http.h"
#include "activities.h"
#include "gpio_config.h"

void setup()
{
    setLed(LED_PRIORITY_CONNECTIVITY, 0x040200);    // Orange (busy)

    Serial.begin(115200);
    delay(2000);
    Serial.println("\n=== ESP32 IR Remote ===");

    setupDeviceName();
    setupGpioConfig();
    setupIrTx();
    setupIrRx();
    setupFs();
    setupActivities();
    setupWifi();
    setupHttp();
    setupBle();

    setLed(LED_PRIORITY_CONNECTIVITY, 0x000400);    // Green (ready)
}

void loop()
{
    pollSerial();
    pollGpio();
    pollIrRx();
    pollIrTx();
    pollWifi();
    pollHttp();
    pollActivities();
    pollTelnet();
    pollBle();
}
