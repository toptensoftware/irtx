#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include "esp_mac.h"
#include "host/ble_hs.h"
#include "host/ble_hs_id.h"
#include "nvs.h"

#include "config.h"
#include "device.h"
#include "ble.h"

#define MAX_BLE_DEVICES     4

/*
Needs PR branch NimBLE library.

$ arduino-cli config set library.enable_unsafe_install true
$ arduino-cli lib install --git-url https://github.com/h2zero/NimBLE-Arduino#nvs-custom-namespace
*/

// BLE server
NimBLEServer*         bleServer      = nullptr;
NimBLEHIDDevice*      hidDevice      = nullptr;
NimBLECharacteristic* keyboardReport = nullptr;
NimBLECharacteristic* consumerReport = nullptr;
NimBLECharacteristic* mouseReport = nullptr;


// Peer addresses
NimBLEAddress peerAddress;

static char slotName[80];
static char nvsName[15];
int activeSlot = -1;
bool isPairing = false;


// ---- HID Report Descriptor: Keyboard (ID=1) + Consumer Control (ID=2) ----
static const uint8_t hidReportDescriptor[] = 
{
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

    // Mouse — Report ID 3
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x03,        //   Report ID (3)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x05, 0x09,        //     Usage Page (Buttons)
    0x19, 0x01,        //     Usage Minimum (1)
    0x29, 0x03,        //     Usage Maximum (3)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x02,        //     Input (Data,Var,Abs) — 3 buttons
    0x75, 0x05,        //     Report Size (5)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x01,        //     Input (Const) — padding
    0x05, 0x01,        //     Usage Page (Generic Desktop)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x09, 0x38,        //     Usage (Wheel)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x06,        //     Input (Data,Var,Rel) — X, Y, Wheel
    0xC0,              //   End Collection
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
bool loadPeerAddress(int slot, NimBLEAddress& addr) 
{
    // Setup key
    char key[8];
    snprintf(key, sizeof(key), "peer%d", slot);
    uint8_t buf[7];

    // Read prefs
    bool valid = false;
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
        addr = NimBLEAddress(buf, buf[6]);
        valid = true;
    }
    prefs.end();

    return valid;
}

void savePeerAddress(int slot, NimBLEAddress addr) 
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

void clearPeerAddress(int slot) 
{
    prefs.begin("ble", false);
    char key[8];
    snprintf(key, sizeof(key), "peer%d", slot);
    prefs.remove(key);
    prefs.end();
}

void clearWhiteList()
{
    if (NimBLEDevice::getWhiteListCount())
    {
        LOG("Clearing white list...\n");
        while (NimBLEDevice::getWhiteListCount())
        {
            NimBLEDevice::whiteListRemove(NimBLEDevice::getWhiteListAddress(0));
        }
        LOG("White list cleared.\n");
    }
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

        // After IRK exchange, peer_id_addr in the connection descriptor is
        // updated to the stable identity address (not the rotating RPA).
        ble_gap_conn_desc desc;
        NimBLEAddress identityAddr = connInfo.getAddress(); // fallback to OTA addr
        if (ble_gap_conn_find(connInfo.getConnHandle(), &desc) == 0)
            identityAddr = NimBLEAddress(desc.peer_id_addr);

        if (isPairing)
        {
            peerAddress = identityAddr;
            savePeerAddress(activeSlot, identityAddr);
            LOG("BLE: slot %d paired to %s\n", activeSlot, identityAddr.toString().c_str());
            isPairing = false;
        }
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

    sprintf(slotName, "%s-%i", deviceName, slot);
    sprintf(nvsName, "nimble_bond_%i", slot);
//    NIMBLE_NVS_NAMESPACE = nvsName;
//    set_nvs_namespace(nvsName);
    set_nimble_nvs_namespace(nvsName);

    LOG("BLE starting (%s)...\n", slotName);


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
    bleServer->advertiseOnDisconnect(false);

    // Create HID device
    hidDevice = new NimBLEHIDDevice(bleServer);
    hidDevice->setManufacturer("Topten Software");
    hidDevice->setPnp(0x02, 0x0000, 0x0000, 0x0001);
    hidDevice->setHidInfo(0x00, 0x02); // country=0, flags=normally-connectable
    hidDevice->setReportMap((uint8_t*)hidReportDescriptor, sizeof(hidReportDescriptor));

    // Get reports
    keyboardReport = hidDevice->getInputReport(1);
    consumerReport = hidDevice->getInputReport(2);
    mouseReport = hidDevice->getInputReport(3);

    // Start 
    hidDevice->startServices();
    bleServer->start(); 

    LOG("BLE started.\n");

    // Initialize advertising
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->setAppearance(GENERIC_HID);
    adv->setName(slotName);
    adv->addServiceUUID(hidDevice->getHidService()->getUUID());
    adv->setConnectableMode(BLE_GAP_CONN_MODE_UND);

}


void stopServer()
{
    if (activeSlot >= 0)
    {
        LOG("BLE Stopping...\n");

        // Disable re-advertise since we're trying to shutdown
        bleServer->advertiseOnDisconnect(true);
        
        // Reset white list
        clearWhiteList();

        // Disconnect
        if (bleServer->getConnectedCount() > 0)
        {
            LOG("BLE disconnecting...\n");
            bleServer->disconnect(bleServer->getPeerInfo(0));
            LOG("BLE disconnected.\n");
        }

        // Stop advertising
        NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
        if (adv->isAdvertising())
        {
            LOG("BLE stopping advertising...\n");
            adv->stop();
            delay(100);
            LOG("BLE advertising stopped.\n");
        }

        LOG("Pausing...\n");
//        for (int i=0; adv->isAdvertising() || bleServer->getConnectedCount() || i<20; i++)
        for (int i=0; i<20; i++)
            delay(5);
        LOG("Continuing...\n");

        // Reset
        LOG("BLE deiniting...\n");
        NimBLEDevice::deinit(true);
        bleServer = nullptr;
        hidDevice = nullptr;
        activeSlot = -1;
        LOG("BLE deinited.\n");

        LOG("BLE Stopped.\n");
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
    PRINT("--- BLE ---\n");

    for (int i = 0; i < MAX_BLE_DEVICES; i++)
    {
        ble_addr_t mac;
        generate_slot_address(i, &mac);

        PRINT("Slot %d      : id: %02X:%02X:%02X:%02X:%02X:%02X",
                i, mac.val[5], mac.val[4], mac.val[3], mac.val[2], mac.val[1], mac.val[0]);

        NimBLEAddress addr;
        if (loadPeerAddress(i, addr))
        {
            auto val = addr.getVal();
            PRINT("  peer: %02X:%02X:%02X:%02X:%02X:%02X",
                 val[5], val[4], val[3], val[2], val[1], val[0]);
        }

        if (i == activeSlot)
        {
            if (bleServer->getConnectedCount() > 0)
                PRINT(" Connected");
            else if (NimBLEDevice::getAdvertising()->isAdvertising())
                PRINT(" Advertising");
        }

        PRINT("\n");
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
    isPairing = true;
    bleServer->advertiseOnDisconnect(false);
    if (!adv->start(0))
    {
        LOG("Failed to start advertising.\n");
        stopServer();
        return;
    }

    LOG("Advertising for pairing...\n");
}

void bleUnpair(int slot)
{
    if (slot < 0 || slot >= MAX_BLE_DEVICES) 
    {
        LOG("Usage: pair <0-%d>\n", MAX_BLE_DEVICES - 1);
        return;
    }

    // Get the peer address
    NimBLEAddress addr;
    if (!loadPeerAddress(slot, addr))
    {
        LOG("Slot %i not paired.\n", slot);
        return;
    }

    // Stop the server if currently connected on this slot
    if (activeSlot == slot)
    {
        stopServer();
    }

    // Clear bond store for this slot
    char sz[16];
    sprintf(sz, "nimble_bond_%i", slot);
    nvs_handle_t handle;
    esp_err_t err = nvs_open(sz, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        nvs_erase_all(handle);
        nvs_commit(handle);
        nvs_close(handle);
    }

    // Delete bond
    clearPeerAddress(slot);

    LOG("Unpaired slot %d\n", slot);
}

void bleConnect(int slot)
{
    // Stop?
    if (slot < 0)
    {
        stopServer();
        return;
    }

    // Invalid?
    if (slot >= MAX_BLE_DEVICES) 
    {
        LOG("Usage: pair <0-%d>\n", MAX_BLE_DEVICES - 1);
        return;
    }

    // Redundant?
    if (activeSlot == slot)
    {
        LOG("Slot %d already connected\n", slot);
        return;
    }

    // Load the paired peer addres
    if (!loadPeerAddress(slot, peerAddress))
    {
        LOG("No paired device for slot %d\n", slot);
        return;
    }

    // Start server
    startServer(slot);

    // Setup white list
    NimBLEDevice::whiteListAdd(peerAddress);

    // Start advertising
    isPairing = false;
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->setScanFilter(false, true);  // connections from whitelist only
    adv->setConnectableMode(BLE_GAP_CONN_MODE_UND);
    if (!adv->start(0))
    {
        LOG("Failed to start advertising.\n");
        stopServer();
        return;
    }
    bleServer->advertiseOnDisconnect(true);
    adv->start(0);


    LOG("Advertising for connection to %s...\n", peerAddress.toString().c_str());
}

void handleBleConnectPacket(uint8_t* data, int length)
{
    if (length < 3) {
        LOG("CONNECT: packet too short\n");
        return;
    }
    uint8_t bleDevIdx = data[2];

    VERBOSE("ble: connecting to %i...\n", bleDevIdx);

    if (bleDevIdx < MAX_BLE_DEVICES)
        bleConnect(bleDevIdx);
    else
        bleConnect(-1);
}

void handleBleHidPacket(uint8_t* data, int length)
{
    uint8_t bleDevIdx  = data[2];
    uint8_t reportId   = data[3];
    uint8_t* reportData = data + 4;
    int      reportLen  = length - 4;

    // Check index — 0xFF means "whatever is connected"
    if (bleDevIdx != 0xFF && bleDevIdx >= MAX_BLE_DEVICES) {
        LOG("HID: device index out of range\n");
        return;
    }

    // Only deliver to the currently pre-connected (desired) device
    if (bleDevIdx != 0xFF && (int)bleDevIdx != activeSlot) {
        LOG("HID: packet for device %i dropped, currently connected to slot %i\n",
            (int)bleDevIdx, (int)activeSlot);
        return;
    }

    // Already connected — send immediately
    if (bleServer->getConnectedCount() ==0) 
    {
        LOG("HID: packet for device %i dropped - not connected\n",
            (int)bleDevIdx);
        return;
    }

    if (reportId == 1 && reportLen == 8) 
    {
        VERBOSE("BLE: sending keyboard report (%02x %02x %02x %02x %02x %02x %02x %02x)\n",
            reportData[0], reportData[1], reportData[2], reportData[3],
            reportData[4], reportData[5], reportData[6], reportData[7]
            );

        keyboardReport->setValue(reportData, reportLen);
        keyboardReport->notify();
    } 
    else if (reportId == 2 && reportLen == 2) 
    {
        VERBOSE("BLE: sending consumer report (%02x %02x)\n", reportData[0], reportData[1]);

        consumerReport->setValue(reportData, reportLen);
        consumerReport->notify();
    }
    else if (reportId == 3 && reportLen == 4) 
    {
        VERBOSE("BLE: sending mouse report (%02x %02x %02x %02x)\n", reportData[0], reportData[1], reportData[2], reportData[3]);
        
        mouseReport->setValue(reportData, reportLen);
        mouseReport->notify();
    }
    else
    {
        LOG("BLE: dropping unrecognized hid packet with reportId: %d length: %d\n", (int)reportId, (int)reportLen);
    }
}
