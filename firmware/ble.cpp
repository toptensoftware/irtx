#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include "esp_mac.h"
#include "host/ble_hs.h"
#include "host/ble_hs_id.h"

#include "config.h"
#include "device.h"
#include "ble.h"

#define MAX_BLE_DEVICES     4

// BLE server
NimBLEServer*         bleServer      = nullptr;
NimBLEHIDDevice*      hidDevice      = nullptr;
NimBLECharacteristic* keyboardReport = nullptr;
NimBLECharacteristic* consumerReport = nullptr;

// Peer addresses
NimBLEAddress peerAddress;
uint8_t       peerAddrType = 0;
bool          peerValid = false;

int activeSlot = -1;
bool isPairing = false;


// ---- HID Report Descriptor: Keyboard (ID=1) + Consumer Control (ID=2) ----
static const uint8_t hidReportDescriptor[] = {
    // Keyboard — Report ID 1
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x05, 0x07,        //   Usage Page (Keyboard/Keypad)
    0x19, 0xE0,        //   Usage Minimum (Left Control)
    0x29, 0xE7,        //   Usage Maximum (Right GUI)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs) — modifier byte
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Const) — reserved byte
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x73,        //   Logical Maximum (115)
    0x05, 0x07,        //   Usage Page (Keyboard/Keypad)
    0x19, 0x00,        //   Usage Minimum (0)
    0x29, 0x73,        //   Usage Maximum (115)
    0x81, 0x00,        //   Input (Data,Array,Abs) — 6 keycodes
    0xC0,              // End Collection
    // Consumer Control — Report ID 2
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x02,        //   Report ID (2)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x03,  //   Logical Maximum (1023)
    0x19, 0x00,        //   Usage Minimum (0)
    0x2A, 0xFF, 0x03,  //   Usage Maximum (1023)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x00,        //   Input (Data,Array,Abs)
    0xC0,              // End Collection
};

// arduino-esp32 v3.3.7 checks btInUse() at startup; if false, it releases BT memory
// before NimBLE can use it, causing a controller-invalid-state crash.
// NimBLE-Arduino 2.3.7 doesn't define this (fixed in NimBLE-Arduino post-2.3.7 via PR #1090).
// Defining it here as a strong symbol overrides the weak default that returns false.
#if SOC_BT_SUPPORTED
bool btInUse() { return true; }
#endif

// Generate the public static random address for a pairing slot on this device
void generate_slot_address(int slot, ble_addr_t *addr) 
{
    uint8_t base_mac[6];
    esp_read_mac(base_mac, ESP_MAC_BT);

    addr->type = BLE_ADDR_RANDOM;

    // Simple deterministic generator: combine MAC and slot
    for (int i = 0; i < 6; i++) 
    {
        addr->val[i] = base_mac[i] ^ ((slot + 1) * 0x55); // simple xor & slot mix
    }

    // Make it a static random address: top two bits = 1 1
    addr->val[5] |= 0xC0;
}


// ---- BLE NVS Helpers ----

// Stores 7 bytes per slot: 6-byte address + 1-byte address type
void loadBleAddress(int slot) 
{
    // Reset current peer address
    peerAddrType = 0;
    peerValid = false;

    // Setup key
    char key[8];
    snprintf(key, sizeof(key), "peer%d", slot);
    uint8_t buf[7];

    // Read prefs
    prefs.begin("ble", true);
    if (prefs.getBytes(key, buf, 7) == 7) 
    {
        for (int j = 0; j < 3; j++) 
        {
            // reverse bytes
            uint8_t temp = buf[j];
            buf[j] = buf[5-j];
            buf[5-j] = temp;
        }
        peerAddress = NimBLEAddress(buf, buf[6]);
        peerAddrType = buf[6];
        peerValid = true;
    }
    prefs.end();
}

void saveBleAddress(int slot, NimBLEAddress addr) 
{
    prefs.begin("ble", false);
    char key[8];
    snprintf(key, sizeof(key), "peer%d", slot);
    uint8_t buf[7];
    memcpy(buf, addr.getVal(), 6);
    buf[6] = addr.getType();
    prefs.putBytes(key, buf, 7);
    prefs.end();
}

void clearBleAddress(int slot) 
{
    prefs.begin("ble", false);
    char key[8];
    snprintf(key, sizeof(key), "peer%d", slot);
    prefs.remove(key);
    prefs.end();
    peerValid = false;
}

// ---- BLE Server Callbacks ----
class BleCallbacks : public NimBLEServerCallbacks 
{
    virtual void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override 
    {
        LOG("BLE: onConnect: %s\n", connInfo.getAddress().toString().c_str());
    }

    virtual void onAuthenticationComplete(NimBLEConnInfo& connInfo) override 
    {
        LOG("BLE: onAuthenticationComplete: %s\n", connInfo.getAddress().toString().c_str());
    }

    virtual void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo, int reason) override 
    {
        LOG("BLE: onDisconnect: %s (reason %d)\n", connInfo.getAddress().toString().c_str(), reason);
    }
};


void stopServer();

void startServer(int slot)
{
    stopServer();

    // Setup active slot
    activeSlot = slot;
    if (activeSlot == -1)
        return;

    char slotName[64];
    sprintf(slotName, "%s-%i", deviceName, slot);
    NimBLEDevice::init(slotName);
    NimBLEDevice::setSecurityAuth(true, true, true); // bonding=true, MITM=true, SC=true

    // Distribute and request both the encryption key and the IRK (identity key).
    // Without BLE_SM_PAIR_KEY_DIST_ID, NimBLE never exchanges IRKs, so it can't
    // resolve iOS's rotating random addresses (RPAs) on subsequent connections.
    NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
    NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);

    // Setup public address for this slot
    ble_addr_t addr;
    generate_slot_address(activeSlot, &addr);
    NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM);
    NimBLEDevice::setOwnAddr(addr.val);

    // Create server
    bleServer = NimBLEDevice::createServer();
    bleServer->setCallbacks(new BleCallbacks());
    bleServer->advertiseOnDisconnect(false); // we manage advertising manually

    // Create HID device
    hidDevice = new NimBLEHIDDevice(bleServer);
    hidDevice->setManufacturer("Topten Software");
    hidDevice->setPnp(0x02, 0x0000, 0x0000, 0x0001);
    hidDevice->setHidInfo(0x00, 0x02); // country=0, flags=normally-connectable
    hidDevice->setReportMap((uint8_t*)hidReportDescriptor, sizeof(hidReportDescriptor));

    // Get reports
    keyboardReport = hidDevice->getInputReport(1);
    consumerReport = hidDevice->getInputReport(2);

    // Start 
    hidDevice->startServices();
    bleServer->start(); 
}

void stopServer()
{
    if (activeSlot >= 0)
    {
        NimBLEDevice::deinit(true);
        bleServer = nullptr;
        hidDevice = nullptr;
        activeSlot = -1;
    }
}


void setupBle()
{
}

void pollBle()
{
    if (!bleServer)
        return;
}


void statusBle()
{
    Serial.println("--- BLE ---");
    /*
    if (connectedDeviceIndex >= 0)
        Serial.printf("Status      : connected (slot %d)\n", connectedDeviceIndex);
    else if (pairingSlot >= 0)
        Serial.printf("Status      : pairing (slot %d, %lus remaining)\n",
                        pairingSlot, (PAIRING_TIMEOUT_MS - (millis() - pairingStartMs)) / 1000);
    else if (desiredDeviceIndex >= 0)
        Serial.printf("Status      : advertising for slot %d\n", desiredDeviceIndex);
    else
        Serial.println("Status      : idle (send cmd=3 to activate)");
    */
    for (int i = 0; i < MAX_BLE_DEVICES; i++) 
    {
        ble_addr_t mac;       
        generate_slot_address(i, &mac);

        Serial.printf("Slot %d      : %02X:%02X:%02X:%02X:%02X:%02X\n",
                i, mac.val[5], mac.val[4], mac.val[3], mac.val[2], mac.val[1], mac.val[0]);

            /*
        if (peerValid[i])
            Serial.printf("Slot %d      : %s%s%s\n", i,
                            peerAddresses[i].toString().c_str(),
                            connectedDeviceIndex == i ? " (connected)" : "",
                            desiredDeviceIndex == i   ? " (desired)"   : "");
        else
            Serial.printf("Slot %d      : (empty)\n", i);
            */
    }
}


void blePair(int slot)
{
    // Check slot
    if (slot < 0 || slot >= MAX_BLE_DEVICES) 
    {
        LOG("Usage: pair <0-%d>\n", MAX_BLE_DEVICES - 1);
        return;
    }

    // Recreate server
    startServer(slot);

    // Start advertising
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->setScanFilter(false, false);
    adv->setConnectableMode(BLE_GAP_CONN_MODE_UND);
    if (!adv->start(0))
    {
        LOG("Failed to start advertising");
        activeSlot = -1;
        return;
    }

    LOG("Advertising...");
}

void bleUnpair(int slot)
{
    if (slot < 0 || slot >= MAX_BLE_DEVICES) 
    {
        LOG("Usage: pair <0-%d>\n", MAX_BLE_DEVICES - 1);
        return;
    }

    // Disconnect
    if (bleServer->getConnectedCount() > 0)
        bleServer->disconnect(bleServer->getPeerInfo(0));

    // Stop advertising
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->stop();

    // Delete bond
    if (peerValid)
        NimBLEDevice::deleteBond(peerAddress);

    // Clear
    clearBleAddress(slot);
    activeSlot = -1;

    LOG("Unpaired device %d\n", slot);
}
