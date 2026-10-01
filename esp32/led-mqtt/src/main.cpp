#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <esp_task_wdt.h>
#include <sys/time.h>
#include <time.h>

#include "secrets.h"
#include "root_ca.h"

#define FW_VERSION "1.1.0"

// Onboard blue LED and BOOT button on most ESP32 DevKit boards
#define LED_PIN 2
#define BUTTON_PIN 0
#define LED_CHANNEL 0

// MQTT topics (TOPIC_PREFIX comes from secrets.h)
//   <prefix>/set     <- "on" / "off" / "toggle"  (simple commands)
//   <prefix>/cmd     <- JSON, see handleCommand()
//   <prefix>/state   -> retained JSON with settings + device info
//   <prefix>/status  -> retained "online" / "offline" (offline is our last will)
//   <prefix>/log     -> JSON log lines: {"seq","ts","up","lvl","msg"[,"hist"]}
const String TOPIC_SET = String(TOPIC_PREFIX) + "/set";
const String TOPIC_CMD = String(TOPIC_PREFIX) + "/cmd";
const String TOPIC_STATE = String(TOPIC_PREFIX) + "/state";
const String TOPIC_STATUS = String(TOPIC_PREFIX) + "/status";
const String TOPIC_LOG = String(TOPIC_PREFIX) + "/log";

const uint32_t WDT_TIMEOUT_S = 30;                   // reboot if loop() hangs this long
const uint32_t OFFLINE_REBOOT_MS = 10 * 60 * 1000;   // reboot if disconnected this long

WiFiClientSecure net;
PubSubClient mqtt(net);
Preferences prefs;

// ---------------------------------------------------------------------------
// Settings (persisted in flash, survive reboots)
// ---------------------------------------------------------------------------

enum Mode { MODE_SOLID, MODE_BLINK, MODE_BREATHE };
const char *MODE_NAMES[] = {"solid", "blink", "breathe"};

struct Settings {
  bool on = false;
  uint8_t brightness = 100;  // 0..100 %
  Mode mode = MODE_SOLID;
  uint16_t periodMs = 1000;  // blink / breathe cycle, 100..10000
  uint16_t reportS = 30;     // state heartbeat interval, 5..3600
} cfg;

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

// ---------------------------------------------------------------------------
// Logging: Serial + ring buffer (replayed on request) + live MQTT
// ---------------------------------------------------------------------------

struct LogLine {
  uint32_t seq;
  uint64_t ts;  // epoch ms, 0 if the clock wasn't synced yet
  uint32_t up;  // ms since boot
  char lvl;     // 'I' info, 'W' warning, 'E' error
  char msg[112];
};

const int LOG_CAPACITY = 60;
LogLine logBuf[LOG_CAPACITY];
uint32_t logSeq = 0;  // total lines ever logged; logBuf holds the last LOG_CAPACITY

uint64_t epochMs() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  if (tv.tv_sec < 1700000000) return 0;
  return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void publishLog(const LogLine &l, bool hist) {
  JsonDocument doc;
  doc["seq"] = l.seq;
  doc["ts"] = l.ts;
  doc["up"] = l.up;
  doc["lvl"] = String(l.lvl);
  doc["msg"] = l.msg;
  if (hist) doc["hist"] = true;
  char out[256];
  size_t n = serializeJson(doc, out, sizeof(out));
  mqtt.publish(TOPIC_LOG.c_str(), (const uint8_t *)out, n, false);
}

void logf(char lvl, const char *fmt, ...) {
  LogLine &l = logBuf[logSeq % LOG_CAPACITY];
  l.seq = ++logSeq;
  l.ts = epochMs();
  l.up = millis();
  l.lvl = lvl;
  va_list args;
  va_start(args, fmt);
  vsnprintf(l.msg, sizeof(l.msg), fmt, args);
  va_end(args);

  Serial.printf("[%c] %s\n", lvl, l.msg);
  if (mqtt.connected()) publishLog(l, false);
}

void replayLogs() {
  uint32_t count = min<uint32_t>(logSeq, LOG_CAPACITY);
  for (uint32_t s = logSeq - count + 1; s <= logSeq; s++) {
    publishLog(logBuf[(s - 1) % LOG_CAPACITY], true);
    mqtt.loop();  // keep the connection serviced while sending a burst
  }
}

// ---------------------------------------------------------------------------
// LED output (PWM so brightness and breathing work)
// ---------------------------------------------------------------------------

void renderLed() {
  float level = 0;  // 0..1 of the configured brightness
  if (cfg.on) {
    uint32_t phase = millis() % cfg.periodMs;
    switch (cfg.mode) {
      case MODE_SOLID: level = 1; break;
      case MODE_BLINK: level = phase < cfg.periodMs / 2 ? 1 : 0; break;
      case MODE_BREATHE: level = (1 - cosf(2 * PI * phase / cfg.periodMs)) / 2; break;
    }
  }
  // Gamma correction so the slider feels linear to the eye
  float b = level * cfg.brightness / 100.0f;
  ledcWrite(LED_CHANNEL, (uint32_t)roundf(powf(b, 2.2f) * 255));
}

// ---------------------------------------------------------------------------
// State reporting
// ---------------------------------------------------------------------------

bool stateDirty = true;

const char *resetReason() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON: return "power-on";
    case ESP_RST_SW: return "restart";
    case ESP_RST_PANIC: return "crash";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT: return "watchdog";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_EXT: return "reset pin";
    case ESP_RST_DEEPSLEEP: return "deep sleep";
    default: return "unknown";
  }
}

void publishState() {
  JsonDocument doc;
  doc["on"] = cfg.on;
  doc["brightness"] = cfg.brightness;
  doc["mode"] = MODE_NAMES[cfg.mode];
  doc["period"] = cfg.periodMs;
  doc["interval"] = cfg.reportS;
  doc["uptime"] = millis() / 1000;
  doc["rssi"] = WiFi.RSSI();
  doc["ssid"] = WiFi.SSID();
  doc["ip"] = WiFi.localIP().toString();
  doc["mac"] = WiFi.macAddress();
  doc["heap"] = ESP.getFreeHeap();
  doc["minHeap"] = ESP.getMinFreeHeap();
  doc["chip"] = ESP.getChipModel();
  doc["cpuMhz"] = ESP.getCpuFreqMHz();
  doc["fw"] = FW_VERSION;
  doc["reset"] = resetReason();
  doc["time"] = epochMs() / 1000;
  char out[640];
  size_t n = serializeJson(doc, out, sizeof(out));
  mqtt.publish(TOPIC_STATE.c_str(), (const uint8_t *)out, n, true);
  stateDirty = false;
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

String pendingTopic, pendingPayload;  // filled by the MQTT callback, handled in loop()
bool wantLogReplay = false;
uint32_t restartAt = 0;

void settingsChanged() {
  saveSettings();
  stateDirty = true;
}

// Simple commands on <prefix>/set
void handleSet(String msg) {
  msg.toLowerCase();
  if (msg == "on") cfg.on = true;
  else if (msg == "off") cfg.on = false;
  else if (msg == "toggle") cfg.on = !cfg.on;
  else {
    logf('W', "Unknown /set command: %s", msg.c_str());
    return;
  }
  logf('I', "LED %s", cfg.on ? "on" : "off");
  settingsChanged();
}

// JSON commands on <prefix>/cmd. Any subset of fields:
//   {"on":true, "brightness":0-100, "mode":"solid|blink|breathe", "period":100-10000, "interval":5-3600}
//   {"action":"restart" | "logs" | "state"}
void handleCommand(const String &payload) {
  JsonDocument doc;
  if (deserializeJson(doc, payload)) {
    logf('W', "Invalid command JSON");
    return;
  }

  bool changed = false;
  if (doc["on"].is<bool>()) {
    cfg.on = doc["on"];
    changed = true;
    logf('I', "LED %s", cfg.on ? "on" : "off");
  }
  if (doc["brightness"].is<int>()) {
    cfg.brightness = constrain(doc["brightness"].as<int>(), 0, 100);
    changed = true;
    logf('I', "Brightness %d%%", cfg.brightness);
  }
  if (doc["mode"].is<const char *>()) {
    String m = doc["mode"].as<const char *>();
    for (int i = 0; i <= MODE_BREATHE; i++) {
      if (m == MODE_NAMES[i]) {
        cfg.mode = (Mode)i;
        changed = true;
        logf('I', "Mode %s", MODE_NAMES[i]);
      }
    }
  }
  if (doc["period"].is<int>()) {
    cfg.periodMs = constrain(doc["period"].as<int>(), 100, 10000);
    changed = true;
    logf('I', "Period %u ms", cfg.periodMs);
  }
  if (doc["interval"].is<int>()) {
    cfg.reportS = constrain(doc["interval"].as<int>(), 5, 3600);
    changed = true;
    logf('I', "Report interval %u s", cfg.reportS);
  }
  if (changed) settingsChanged();

  String action = doc["action"] | "";
  if (action == "restart") {
    logf('W', "Restart requested");
    restartAt = millis() + 1000;  // give the log line time to go out
  } else if (action == "logs") {
    wantLogReplay = true;
  } else if (action == "state") {
    stateDirty = true;
  } else if (action.length()) {
    logf('W', "Unknown action: %s", action.c_str());
  }
}

void onMessage(char *topic, byte *payload, unsigned int len) {
  // Copy out: PubSubClient reuses this buffer for the next publish
  pendingTopic = topic;
  pendingPayload = String((const char *)payload, len);
  pendingPayload.trim();
}

// ---------------------------------------------------------------------------
// Connectivity
// ---------------------------------------------------------------------------

// TLS certificate checks need a correct clock
void syncTime() {
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  for (int i = 0; i < 20 && time(nullptr) < 1700000000; i++) {
    delay(500);
    esp_task_wdt_reset();
  }
  if (time(nullptr) > 1700000000) logf('I', "Clock synced");
  else logf('W', "Clock sync failed");
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  logf('I', "Connecting to WiFi \"%s\"", WIFI_SSID);
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    esp_task_wdt_reset();
  }
}

bool connectMqtt() {
  String clientId = "esp32-" + WiFi.macAddress();
  clientId.replace(":", "");

  // Last will: the broker publishes "offline" for us if we drop off without saying goodbye
  if (!mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS, TOPIC_STATUS.c_str(), 1, true, "offline")) {
    logf('E', "MQTT connect failed (state %d)", mqtt.state());
    return false;
  }
  mqtt.publish(TOPIC_STATUS.c_str(), "online", true);
  mqtt.subscribe(TOPIC_SET.c_str(), 1);
  mqtt.subscribe(TOPIC_CMD.c_str(), 1);
  logf('I', "MQTT connected as %s", clientId.c_str());
  publishState();
  return true;
}

// ---------------------------------------------------------------------------

uint32_t lastConnectedMs = 0;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  ledcSetup(LED_CHANNEL, 5000, 8);
  ledcAttachPin(LED_PIN, LED_CHANNEL);

  esp_task_wdt_init(WDT_TIMEOUT_S, true);
  esp_task_wdt_add(NULL);

  loadSettings();
  logf('I', "Boot, firmware %s, reset reason: %s", FW_VERSION, resetReason());
  logf('I', "Settings: LED %s, %d%%, %s, %u ms", cfg.on ? "on" : "off", cfg.brightness,
       MODE_NAMES[cfg.mode], cfg.periodMs);

  connectWifi();
  if (WiFi.status() == WL_CONNECTED) syncTime();

  net.setCACert(ROOT_CA);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setKeepAlive(30);
  mqtt.setBufferSize(1024);
  lastConnectedMs = millis();
}

void loop() {
  esp_task_wdt_reset();
  uint32_t now = millis();

  // Log WiFi transitions
  static bool wifiWasUp = false;
  bool wifiUp = WiFi.status() == WL_CONNECTED;
  if (wifiUp != wifiWasUp) {
    if (wifiUp) logf('I', "WiFi connected, IP %s, RSSI %d dBm", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    else logf('W', "WiFi disconnected");
    wifiWasUp = wifiUp;
  }

  if (wifiUp && mqtt.connected()) {
    lastConnectedMs = now;
    mqtt.loop();
  } else if (wifiUp) {
    // Retry MQTT every 5 s (connect() blocks for a few seconds on failure)
    static uint32_t lastTry = 0;
    if (now - lastTry > 5000 || lastTry == 0) {
      lastTry = now;
      if (time(nullptr) < 1700000000) syncTime();
      connectMqtt();
    }
  }

  // Handle a command received during mqtt.loop()
  if (pendingTopic.length()) {
    String topic = pendingTopic, payload = pendingPayload;
    pendingTopic = "";
    if (topic == TOPIC_SET) handleSet(payload);
    else if (topic == TOPIC_CMD) handleCommand(payload);
  }
  if (wantLogReplay && mqtt.connected()) {
    wantLogReplay = false;
    replayLogs();
  }
  if (restartAt && now > restartAt) {
    mqtt.publish(TOPIC_STATUS.c_str(), "offline", true);
    mqtt.disconnect();
    delay(100);
    ESP.restart();
  }

  // Safety net: if we've been offline for a long time, start over from scratch
  if (now - lastConnectedMs > OFFLINE_REBOOT_MS) {
    logf('E', "Offline too long, rebooting");
    ESP.restart();
  }

  // BOOT button toggles the LED locally (active low, simple debounce)
  static bool lastPressed = false;
  bool pressed = digitalRead(BUTTON_PIN) == LOW;
  if (pressed && !lastPressed) {
    cfg.on = !cfg.on;
    logf('I', "BOOT button: LED %s", cfg.on ? "on" : "off");
    settingsChanged();
  }
  lastPressed = pressed;

  static uint32_t lastState = 0;
  if (mqtt.connected() && (stateDirty || now - lastState > cfg.reportS * 1000UL)) {
    lastState = now;
    publishState();
  }

  renderLed();
  delay(10);
}
