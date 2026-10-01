// Settings kept in flash: the broker connection, the device identity, remembered WiFi networks
// and the LED settings
#pragma once
#include <Arduino.h>

struct Connection {
  String host, user, pass;  // broker login, shared by all boards
  uint16_t port = 8883;
  String base;              // topic base shared by all boards, e.g. "mivebe"
  String id;                // "esp32-" + last 6 hex digits of the MAC, unique per board
  String name;              // friendly name, may be empty
  String prefix;            // base + "/" + id
};

enum Mode { MODE_SOLID, MODE_BLINK, MODE_BREATHE };
extern const char *MODE_NAMES[];

struct Settings {
  bool on = false;
  uint8_t brightness = 100;  // 0..100 %
  Mode mode = MODE_SOLID;
  uint16_t periodMs = 1000;  // blink / breathe cycle, 100..10000
  uint16_t reportS = 30;     // state heartbeat interval, 5..3600
};

struct WifiNetwork {
  String ssid, pass;
};

const int MAX_NETWORKS = 5;

extern Connection conn;
extern Settings cfg;
extern WifiNetwork networks[MAX_NETWORKS];  // most recently added first
extern int networkCount;

// MQTT topics, all under conn.prefix (<base>/<id>)
//   <prefix>/set     <- "on" / "off" / "toggle"  (simple commands)
//   <prefix>/cmd     <- JSON, see handleCommand()
//   <prefix>/state   -> retained JSON with settings + device info
//   <prefix>/status  -> retained "online" / "offline" (offline is our last will)
//   <prefix>/log     -> JSON log lines: {"seq","ts","up","lvl","msg"[,"hist"]}
//   <prefix>/ota     -> firmware update progress: {"state":"downloading|rebooting|failed","progress","error"}
extern String TOPIC_SET, TOPIC_CMD, TOPIC_STATE, TOPIC_STATUS, TOPIC_LOG, TOPIC_OTA;

void loadConnection();  // also loads the WiFi networks and sets the topics
void setName(const String &name);
void addNetwork(const String &ssid, const String &pass);  // adds or updates, moves it to the front
void forgetNetworks();

void loadSettings();
void saveSettings();
