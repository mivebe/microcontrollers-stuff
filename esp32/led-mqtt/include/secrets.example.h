// Copy this file to secrets.h (git-ignored) and fill in your values.
// A USB upload saves these to the board's flash. OTA builds have no secrets.h and use what's saved.
#pragma once

// Optional: a 2.4 GHz WiFi the board knows from its very first boot (handy at home).
// Only used once; on site the owner adds their WiFi through the setup portal (ESP32-Setup-xxxxxx).
// Remove both lines to ship a board with no WiFi preset.
#define WIFI_SSID "your-2.4GHz-wifi"
#define WIFI_PASS "your-wifi-password"

// HiveMQ Cloud -> your cluster -> Access Management -> create credentials for the devices.
// All boards share this login and are rewritten to it on every USB upload.
#define MQTT_HOST "5a476a1b4c814901b54584ec2ba638c7.s1.eu.hivemq.cloud"
#define MQTT_PORT 8883
#define MQTT_USER "esp32-device"
#define MQTT_PASS "device-password"

// Topic base shared by all boards. Each board adds its own id: <base>/esp32-a1b2c3/...
// The web app's connection settings must use the same base.
#define TOPIC_BASE "mivebe"
