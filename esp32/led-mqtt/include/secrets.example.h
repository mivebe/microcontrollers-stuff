// Copy this file to secrets.h (git-ignored) and fill in your values.
#pragma once

// Home WiFi - must be a 2.4 GHz network (ESP32 has no 5 GHz radio)
#define WIFI_SSID "your-2.4GHz-wifi"
#define WIFI_PASS "your-wifi-password"

// HiveMQ Cloud -> your cluster -> Access Management -> create credentials for the device
#define MQTT_HOST "5a476a1b4c814901b54584ec2ba638c7.s1.eu.hivemq.cloud"
#define MQTT_PORT 8883
#define MQTT_USER "esp32-device"
#define MQTT_PASS "device-password"

// All topics live under this prefix; the web app must use the same one
#define TOPIC_PREFIX "mivebe/esp32-1"
