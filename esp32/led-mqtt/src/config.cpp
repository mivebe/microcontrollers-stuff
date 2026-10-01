#include "config.h"

#include <Preferences.h>
#include <esp_mac.h>

// secrets.h only exists on your PC. The CI build (published for OTA) has none and uses the
// settings a previous USB upload saved to flash, see loadConnection().
#if __has_include("secrets.h")
#include "secrets.h"
#define HAS_SECRETS 1
#else
#define HAS_SECRETS 0
#endif

const char *MODE_NAMES[] = {"solid", "blink", "breathe"};

Connection conn;
Settings cfg;
WifiNetwork networks[MAX_NETWORKS];
int networkCount = 0;
String TOPIC_SET, TOPIC_CMD, TOPIC_STATE, TOPIC_STATUS, TOPIC_LOG, TOPIC_OTA;

static Preferences prefs;

// getString() logs an error for keys that were never written; those are normal on a fresh board
static String getStr(const char *key) { return prefs.isKey(key) ? prefs.getString(key, "") : String(); }

// ---------------------------------------------------------------------------
// WiFi networks ("wifi" namespace: n, s0..s4, p0..p4, seeded)
// ---------------------------------------------------------------------------

static void loadNetworks() {
  prefs.begin("wifi", true);
  networkCount = min<int>(prefs.getUChar("n", 0), MAX_NETWORKS);
  for (int i = 0; i < networkCount; i++) {
    networks[i].ssid = getStr(("s" + String(i)).c_str());
    networks[i].pass = getStr(("p" + String(i)).c_str());
  }
  prefs.end();
}

static void saveNetworks() {
  prefs.begin("wifi", false);
  prefs.putUChar("n", networkCount);
  for (int i = 0; i < networkCount; i++) {
    prefs.putString(("s" + String(i)).c_str(), networks[i].ssid);
    prefs.putString(("p" + String(i)).c_str(), networks[i].pass);
  }
  prefs.end();
}

void addNetwork(const String &ssid, const String &pass) {
  if (ssid.isEmpty()) return;
  // Drop an existing entry for this network, then insert at the front
  int keep = 0;
  WifiNetwork rest[MAX_NETWORKS];
  for (int i = 0; i < networkCount; i++)
    if (networks[i].ssid != ssid) rest[keep++] = networks[i];
  networks[0] = {ssid, pass};
  networkCount = 1;
  for (int i = 0; i < keep && networkCount < MAX_NETWORKS; i++) networks[networkCount++] = rest[i];
  saveNetworks();
}

void forgetNetworks() {
  networkCount = 0;
  saveNetworks();
}

#if HAS_SECRETS && defined(WIFI_SSID)
// The WiFi in secrets.h is only a first-boot default. Networks added on site, and a factory
// reset, are never undone by it.
static void seedNetwork(const char *ssid, const char *pass) {
  prefs.begin("wifi", true);
  bool seeded = prefs.getBool("seeded", false);
  prefs.end();
  if (seeded) return;
  if (strlen(ssid)) addNetwork(ssid, pass);
  prefs.begin("wifi", false);
  prefs.putBool("seeded", true);
  prefs.end();
}
#endif

// ---------------------------------------------------------------------------
// Connection settings + identity ("conn" namespace)
// ---------------------------------------------------------------------------

#if HAS_SECRETS
static void putIfChanged(const char *key, const char *value) {
  if (getStr(key) != value) prefs.putString(key, value);
}
#endif

void loadConnection() {
  loadNetworks();

  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char id[16];
  snprintf(id, sizeof(id), "esp32-%02x%02x%02x", mac[3], mac[4], mac[5]);
  conn.id = id;

  prefs.begin("conn", false);
#if HAS_SECRETS
  // A USB build carries secrets.h: remember the shared broker settings for later OTA builds
  putIfChanged("host", MQTT_HOST);
  putIfChanged("user", MQTT_USER);
  putIfChanged("pass", MQTT_PASS);
  putIfChanged("base", TOPIC_BASE);
  if (prefs.getUShort("port", 0) != MQTT_PORT) prefs.putUShort("port", MQTT_PORT);
#endif

  // Firmware 1.2.0 stored one WiFi network and a full prefix here
  if (prefs.isKey("prefix")) {
    String old = getStr("prefix");
    if (!prefs.isKey("base")) prefs.putString("base", old.substring(0, max(0, old.lastIndexOf('/'))));
    prefs.remove("prefix");
  }
  if (prefs.isKey("ssid")) {
    String ssid = getStr("ssid"), pass = getStr("wifiPass");
    prefs.remove("ssid");
    prefs.remove("wifiPass");
    if (networkCount == 0) addNetwork(ssid, pass);
  }

  conn.host = getStr("host");
  conn.user = getStr("user");
  conn.pass = getStr("pass");
  conn.port = prefs.getUShort("port", 8883);
  conn.base = getStr("base");
  conn.name = getStr("name");
  prefs.end();

#if HAS_SECRETS && defined(WIFI_SSID)
  seedNetwork(WIFI_SSID, WIFI_PASS);
#endif

  conn.prefix = conn.base + "/" + conn.id;
  TOPIC_SET = conn.prefix + "/set";
  TOPIC_CMD = conn.prefix + "/cmd";
  TOPIC_STATE = conn.prefix + "/state";
  TOPIC_STATUS = conn.prefix + "/status";
  TOPIC_LOG = conn.prefix + "/log";
  TOPIC_OTA = conn.prefix + "/ota";
}

void setName(const String &name) {
  conn.name = name.substring(0, 32);
  conn.name.trim();
  prefs.begin("conn", false);
  prefs.putString("name", conn.name);
  prefs.end();
}

// ---------------------------------------------------------------------------
// LED settings (persisted in flash, survive reboots)
// ---------------------------------------------------------------------------

void loadSettings() {
  prefs.begin("led", true);
  cfg.on = prefs.getBool("on", cfg.on);
  cfg.brightness = prefs.getUChar("bright", cfg.brightness);
  cfg.mode = (Mode)prefs.getUChar("mode", cfg.mode);
  cfg.periodMs = prefs.getUShort("period", cfg.periodMs);
  cfg.reportS = prefs.getUShort("report", cfg.reportS);
  prefs.end();
  if (cfg.mode > MODE_BREATHE) cfg.mode = MODE_SOLID;
}

void saveSettings() {
  prefs.begin("led", false);
  prefs.putBool("on", cfg.on);
  prefs.putUChar("bright", cfg.brightness);
  prefs.putUChar("mode", cfg.mode);
  prefs.putUShort("period", cfg.periodMs);
  prefs.putUShort("report", cfg.reportS);
  prefs.end();
}
