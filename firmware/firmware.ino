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
#include <WiFiUdp.h>
#include <Preferences.h>
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "esp_mac.h"

// ---- Logging ----
// Operational log messages prefixed with seconds since boot.
#define LOG(fmt, ...) Serial.printf("[%7.3f] " fmt, millis() / 1000.0f, ##__VA_ARGS__)

// ---- Configuration ----
#define UDP_PORT     4210
#define IR_TX_PIN    4
#define LED_PIN      10       // Onboard WS2812 RGB LED
#define CARRIER_FREQ 38000   // 38kHz IR carrier

#define MAX_TIMING_VALUES  512
#define MAX_IR_DEVICES     16

// ---- IR Globals ----
Preferences prefs;
WiFiUDP udp;
rmt_channel_handle_t txChannel   = NULL;
rmt_encoder_handle_t copyEncoder = NULL;

// ---- Device State ----
struct IrDeviceState { int64_t availableTime; };
IrDeviceState irDeviceStates[MAX_IR_DEVICES] = {};

// ---- Packet Decoding ----
#define IR_HEADER_SIZE 12  // uint16 cmd + uint16 irDevIdx + uint32 carrierFreq + uint32 gap
uint8_t  packetBuffer[IR_HEADER_SIZE + MAX_TIMING_VALUES * 2];
uint16_t timingValues[MAX_TIMING_VALUES + 1];
rmt_symbol_word_t rmtSymbols[MAX_TIMING_VALUES];
int rmtSymbolCount = 0;

// ---- Device Name ----
static char deviceName[64] = "urem";

void loadDeviceName() 
{
    prefs.begin("device", true);
    String n = prefs.getString("name", "urem");
    prefs.end();
    n.trim();
    if (n.length() > 0 && n.length() < sizeof(deviceName))
        strncpy(deviceName, n.c_str(), sizeof(deviceName) - 1);
}


// ---- Status LED (onboard WS2812) ----
void ledColor(uint8_t r, uint8_t g, uint8_t b) 
{
    neopixelWrite(LED_PIN, r, g, b);
}

// ---- RMT Setup ----
void setupRmt() 
{
    rmt_tx_channel_config_t txConfig = {};
    txConfig.gpio_num = (gpio_num_t)IR_TX_PIN;
    txConfig.clk_src = RMT_CLK_SRC_DEFAULT;
    txConfig.resolution_hz = 1000000; // 1MHz = 1us per tick
    txConfig.mem_block_symbols = 48;
    txConfig.trans_queue_depth = 1;
    ESP_ERROR_CHECK(rmt_new_tx_channel(&txConfig, &txChannel));

    rmt_carrier_config_t carrierConfig = {};
    carrierConfig.frequency_hz = CARRIER_FREQ;
    carrierConfig.duty_cycle = 0.33;
    ESP_ERROR_CHECK(rmt_apply_carrier(txChannel, &carrierConfig));
    ESP_ERROR_CHECK(rmt_enable(txChannel));

    rmt_copy_encoder_config_t encConfig = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&encConfig, &copyEncoder));

    LOG("RMT initialized\n");
}

// ---- Build RMT Symbols ----
void buildRmtSymbols(uint16_t* timings, int count) 
{
    struct Phase { uint8_t level; uint16_t duration; };
    static Phase phases[MAX_TIMING_VALUES * 2];
    int phaseCount = 0;

    for (int i = 0; i < count; i++) 
    {
        uint8_t level = (i % 2 == 0) ? 1 : 0;
        uint16_t remaining = timings[i];
        while (remaining > 0 && phaseCount < MAX_TIMING_VALUES * 2) 
        {
            uint16_t chunk = (remaining > 32767) ? 32767 : remaining;
            phases[phaseCount++] = {level, chunk};
            remaining -= chunk;
        }
    }

    rmtSymbolCount = 0;
    for (int i = 0; i < phaseCount; i += 2) 
    {
        rmtSymbols[rmtSymbolCount].level0    = phases[i].level;
        rmtSymbols[rmtSymbolCount].duration0 = phases[i].duration;
        if (i + 1 < phaseCount) 
        {
            rmtSymbols[rmtSymbolCount].level1    = phases[i+1].level;
            rmtSymbols[rmtSymbolCount].duration1 = phases[i+1].duration;
        } 
        else 
        {
            rmtSymbols[rmtSymbolCount].level1    = 0;
            rmtSymbols[rmtSymbolCount].duration1 = 0;
        }
        rmtSymbolCount++;
    }
}

// ---- IR Packet Handler ----
// [uint16 cmd=1][uint16 irDevIdx][uint32 gap][uint16 timings...]
void handleIrPacket(uint8_t* data, int length) 
{
    if (length < IR_HEADER_SIZE + 2) 
    {
        LOG("IR: packet too short\n");
        return;
    }

    uint16_t irDevIdx;
    uint32_t carrierFreq;
    uint32_t gap;
    memcpy(&irDevIdx,       data + 2, 2);
    memcpy(&carrierFreq,    data + 4, 4);
    memcpy(&gap,            data + 8, 4);

    if (irDevIdx >= MAX_IR_DEVICES) 
    {
        LOG("IR: device index out of range\n");
        return;
    }

    if (carrierFreq != 38000) 
    {
        LOG("IR: unsupported carrier frequency\n");
        return;
    }

    // Copy timing values
    int timingBytes = length - IR_HEADER_SIZE;
    int timingCount = timingBytes / 2;
    memcpy(timingValues, data + IR_HEADER_SIZE, timingBytes);

    // Make sure even count
    if (timingCount % 2 != 0) 
    {
        timingValues[timingCount] = 0;
        timingCount++;
    }

    // Sum up the total length
    int timingLength = 0;
    for (int i=0; i<timingCount; i++)
    {
        timingLength += timingValues[i];
    }

    // Enforce per-device gap between transmissions
    IrDeviceState* dev = &irDeviceStates[irDevIdx];
    int64_t now = esp_timer_get_time();
    if (dev->availableTime > 0 && now < dev->availableTime)
        delayMicroseconds(dev->availableTime - now);
    dev->availableTime = now + timingLength + gap;

    // Setup RMT
    buildRmtSymbols(timingValues, timingCount);
    if (rmtSymbolCount == 0) return;

    // Show activity
    ledColor(0, 64, 0);

    // Transmit
    rmt_transmit_config_t txCfg = {};
    txCfg.loop_count = 0;
    esp_err_t err = rmt_transmit(txChannel, copyEncoder,
                                 rmtSymbols, rmtSymbolCount * sizeof(rmt_symbol_word_t),
                                 &txCfg);
    if (err != ESP_OK) 
    {
        LOG("IR: transmit error: %s\n", esp_err_to_name(err));
        ledColor(4, 0, 0);
        return;
    }
    rmt_tx_wait_all_done(txChannel, portMAX_DELAY);

    // Clear activity
    ledColor(0, 2, 0);
}

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

// ---- Serial Terminal ----
#define INPUT_MAX 256
static char inputLine[INPUT_MAX];
static int  inputLen  = 0;
static bool lastWasCR = false;

void handleCommand(const char* line) 
{
    while (*line == ' ') line++;

    if (strncmp(line, "setwifi ", 8) == 0) 
    {
        const char* p = line + 8;
        while (*p == ' ') p++;
        const char* ssidStart = p;
        while (*p && *p != ' ') p++;
        int ssidLen = p - ssidStart;
        if (ssidLen == 0) { LOG("Usage: setwifi <ssid> <password>\n"); return; }
        char ssid[128];
        ssidLen = min(ssidLen, 127);
        memcpy(ssid, ssidStart, ssidLen); ssid[ssidLen] = '\0';
        while (*p == ' ') p++;
        int passLen = strlen(p);
        while (passLen > 0 && p[passLen-1] == ' ') passLen--;
        char password[256];
        passLen = min(passLen, 255);
        memcpy(password, p, passLen); password[passLen] = '\0';
        prefs.begin("wifi", false);
        prefs.putString("ssid", ssid);
        prefs.putString("password", password);
        prefs.end();
        LOG("Saved WiFi: ssid=\"%s\"\n", ssid);
        LOG("Reconnecting...\n");
        WiFi.disconnect();
        setupWifi();
        if (WiFi.status() == WL_CONNECTED) 
        {
            udp.begin(UDP_PORT);
            LOG("Listening for UDP on port %d\n", UDP_PORT);
        }
        ledColor(0, 4, 0);

    } 
    else if (strncmp(line, "name ", 5) == 0) 
    {
        const char* p = line + 5;
        while (*p == ' ') p++;
        int nameLen = strlen(p);
        while (nameLen > 0 && p[nameLen-1] == ' ') nameLen--;
        if (nameLen == 0 || nameLen >= (int)sizeof(deviceName)) 
        {
            LOG("Usage: name <devicename> (1-%d chars)\n", (int)sizeof(deviceName) - 1);
            return;
        }
        char newName[64];
        memcpy(newName, p, nameLen); newName[nameLen] = '\0';
        prefs.begin("device", false);
        prefs.putString("name", newName);
        prefs.end();
        LOG("Device name set to \"%s\" — restart to apply\n", newName);

    } 
    else if (strcmp(line, "status") == 0) 
    {
        // Device
        Serial.printf("Device name : %s\n", deviceName);
        // WiFi
        Serial.println("--- WiFi ---");
        prefs.begin("wifi", true);
        String ssid = prefs.getString("ssid", "(not set)");
        prefs.end();
        Serial.printf("SSID        : %s\n", ssid.c_str());
        if (WiFi.status() == WL_CONNECTED) 
        {
            Serial.printf("IP address  : %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("Gateway     : %s\n", WiFi.gatewayIP().toString().c_str());
            Serial.printf("RSSI        : %d dBm\n", WiFi.RSSI());
        } 
        else 
        {
            Serial.println("Status      : disconnected");
        }
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        Serial.printf("MAC (WiFi)  : %02X:%02X:%02X:%02X:%02X:%02X\n",
                      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        Serial.printf("UDP port    : %d\n", UDP_PORT);
    } 
    else if (*line == '\0') 
    {
        // ignore empty line

    } 
    else 
    {
        LOG("Unknown command: %s\n", line);
        LOG("Commands: setwifi <ssid> <password> | name <devicename> | pair <0-3> | unpair <0-3> | status\n");
    }
}

void handleSerialInput() 
{
    while (Serial.available()) 
    {
        char c = Serial.read();
        if (c == '\n' && lastWasCR) 
        { 
            lastWasCR = false; 
            continue; 
        }
        lastWasCR = (c == '\r');
        if (c == '\r' || c == '\n') 
        {
            Serial.println();
            inputLine[inputLen] = '\0';
            handleCommand(inputLine);
            inputLen = 0;
        } 
        else if (c == '\b' || c == 127) 
        {
            if (inputLen > 0) 
            { 
                inputLen--; Serial.print("\b \b"); 
            }
        } 
        else if (c >= 32 && inputLen < INPUT_MAX - 1) 
        {
            inputLine[inputLen++] = c;
            Serial.print(c);
        }
    }
}

// ---- Arduino Entry Points ----
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
    // Serial 
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

    // UDP packets
    if (WiFi.status() == WL_CONNECTED) 
    {
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
}
