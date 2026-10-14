#include "ble_mouse.h"

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include <BLEServer.h>
#include <esp_gap_ble_api.h>
#include <esp_mac.h>

namespace {

// Report ID 1: 5 buttons, 16-bit X/Y, wheel, horizontal wheel, the layout of current mainstream
// mice. (The 8-bit X/Y layout most hobby libraries send is a recognizable DIY fingerprint.)
const uint8_t REPORT_MAP[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x05, 0x09,        //     Usage Page (Button)
    0x19, 0x01,        //     Usage Minimum (1)
    0x29, 0x05,        //     Usage Maximum (5)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x95, 0x05,        //     Report Count (5)
    0x75, 0x01,        //     Report Size (1)
    0x81, 0x02,        //     Input (Data, Variable, Absolute)
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x03,        //     Report Size (3)
    0x81, 0x01,        //     Input (Constant): padding
    0x05, 0x01,        //     Usage Page (Generic Desktop)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x16, 0x01, 0x80,  //     Logical Minimum (-32767)
    0x26, 0xFF, 0x7F,  //     Logical Maximum (32767)
    0x75, 0x10,        //     Report Size (16)
    0x95, 0x02,        //     Report Count (2)
    0x81, 0x06,        //     Input (Data, Variable, Relative)
    0x09, 0x38,        //     Usage (Wheel)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x06,        //     Input (Data, Variable, Relative)
    0x05, 0x0C,        //     Usage Page (Consumer)
    0x0A, 0x38, 0x02,  //     Usage (AC Pan)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x06,        //     Input (Data, Variable, Relative)
    0xC0,              //   End Collection
    0xC0,              // End Collection
};

BLEHIDDevice *hid = nullptr;
BLECharacteristic *input = nullptr;
BLECharacteristic *battery = nullptr;
volatile bool connected = false;
uint8_t address[6];

void setNotify(BLECharacteristic *c, bool on) {
  auto *cccd = (BLE2902 *)c->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  if (cccd) cccd->setNotifications(on);
}

class ServerCallbacks : public BLEServerCallbacks {
  // The client config isn't stored with the bond, and a bonded computer doesn't write it again on
  // reconnect, so switch notifications on ourselves or the mouse would go silent after a reconnect.
  void onConnect(BLEServer *) override {
    setNotify(input, true);
    setNotify(battery, true);
    connected = true;
  }
  void onDisconnect(BLEServer *) override {
    connected = false;
    setNotify(input, false);
    BLEDevice::startAdvertising();  // so the computer can reconnect
  }
};

// Real BLE mice use a random static address (top two bits set), stable for the device. Derive one
// from the chip's MAC so it's the same on every boot but doesn't carry Espressif's vendor prefix.
void makeAddress() {
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  address[0] = 0xC0 | ((mac[3] ^ mac[5]) & 0x3F);
  address[1] = mac[4] ^ 0x9E;
  address[2] = mac[5] ^ 0x37;
  address[3] = mac[3] ^ 0x5B;
  address[4] = mac[4];
  address[5] = mac[5];
}

}  // namespace

void mouseBegin(const MouseIdentity &id, uint8_t batteryLevel) {
  BLEDevice::init(id.name);
  esp_ble_gap_config_local_icon(ESP_BLE_APPEARANCE_HID_MOUSE);

  // Secure Connections pairing with no PIN ("Just Works"), like any mouse
  BLESecurity security;
  security.setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
  security.setCapability(ESP_IO_CAP_NONE);
  security.setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  security.setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  hid = new BLEHIDDevice(server);
  input = hid->inputReport(1);
  hid->manufacturer()->setValue(id.manufacturer);
  hid->deviceInfo()->createCharacteristic((uint16_t)0x2A24, BLECharacteristic::PROPERTY_READ)
      ->setValue(id.model);
  hid->deviceInfo()->createCharacteristic((uint16_t)0x2A26, BLECharacteristic::PROPERTY_READ)
      ->setValue(id.firmware);
  hid->pnp(id.vendorIdSource, id.vendorId, id.productId, id.version);
  hid->hidInfo(0x00, 0x02);  // not localized, normally connectable
  hid->reportMap((uint8_t *)REPORT_MAP, sizeof(REPORT_MAP));
  battery = hid->batteryService()->getCharacteristic(BLEUUID((uint16_t)0x2A19));  // for setNotify
  hid->startServices();
  hid->setBatteryLevel(batteryLevel);  // also notifies, so only once the services run

  makeAddress();
  BLEAdvertising *adv = server->getAdvertising();
  adv->setDeviceAddress(address, BLE_ADDR_TYPE_RANDOM);
  adv->setAppearance(ESP_BLE_APPEARANCE_HID_MOUSE);
  adv->addServiceUUID(hid->hidService()->getUUID());
  adv->setScanResponse(true);
  adv->start();
}

bool mouseConnected() { return connected; }

void mouseMove(int16_t dx, int16_t dy) {
  if (!connected) return;
  uint8_t report[7] = {0, (uint8_t)dx, (uint8_t)(dx >> 8), (uint8_t)dy, (uint8_t)(dy >> 8), 0, 0};
  input->setValue(report, sizeof(report));
  input->notify();
}

void mouseSetBattery(uint8_t level) {
  if (!hid) return;
  hid->setBatteryLevel(level);  // notifies the computer too
}

String mouseAddress() {
  char s[18];
  snprintf(s, sizeof(s), "%02X:%02X:%02X:%02X:%02X:%02X", address[0], address[1], address[2],
           address[3], address[4], address[5]);
  return s;
}
