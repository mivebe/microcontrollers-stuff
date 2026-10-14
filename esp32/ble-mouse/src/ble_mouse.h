// Bluetooth LE HID mouse, built so the computer sees an ordinary mouse: standard report layout,
// random static address, Device Information with the identity below, battery notifications.
#pragma once
#include <Arduino.h>

struct MouseIdentity {
  const char *name;           // shown when pairing
  const char *manufacturer;   // Device Information strings
  const char *model;
  const char *firmware;
  uint8_t vendorIdSource;     // 0x01 = Bluetooth SIG id, 0x02 = USB-IF id
  uint16_t vendorId, productId, version;
};

void mouseBegin(const MouseIdentity &id, uint8_t battery);
bool mouseConnected();
void mouseMove(int16_t dx, int16_t dy);
void mouseSetBattery(uint8_t level);
String mouseAddress();
