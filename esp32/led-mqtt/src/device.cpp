#include "device.h"

#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_task_wdt.h>
#include <time.h>

#include "config.h"
#include "logger.h"
#include "ota.h"
#include "root_ca.h"
#include "version.h"
#include "wifi_setup.h"

static WiFiClientSecure net;
PubSubClient mqtt(net);

bool stateDirty = true;
uint32_t restartAt = 0;

static String pendingTopic, pendingPayload;  // filled by the MQTT callback, handled in handleMessages()
static bool wantLogReplay = false;

// ---------------------------------------------------------------------------
// LED output (PWM so brightness and breathing work)
// ---------------------------------------------------------------------------

void renderLed() {
  // Double blink at full brightness: "needs WiFi setup", whatever the LED settings are
  if (wifiPortalOpen()) {
    uint32_t phase = millis() % 1500;
    ledcWrite(LED_CHANNEL, phase < 120 || (phase >= 300 && phase < 420) ? 255 : 0);
    return;
  }

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
  doc["id"] = conn.id;
  doc["name"] = conn.name;
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
  char out[768];
  size_t n = serializeJson(doc, out, sizeof(out));
  mqtt.publish(TOPIC_STATE.c_str(), (const uint8_t *)out, n, true);
  stateDirty = false;
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

void settingsChanged() {
  saveSettings();
  stateDirty = true;
}

// Simple commands on <prefix>/set
static void handleSet(String msg) {
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
//   {"name":"Garage"}  (up to 32 characters, "" clears it)
//   {"action":"restart" | "logs" | "state"}
//   {"action":"update", "url":"https://.../firmware-x.y.z.bin"}
static void handleCommand(const String &payload) {
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
  if (doc["name"].is<const char *>()) {
    setName(doc["name"].as<const char *>());
    logf('I', "Name \"%s\"", conn.name.c_str());
    stateDirty = true;
  }

  String action = doc["action"] | "";
  if (action == "restart") {
    logf('W', "Restart requested");
    restartAt = millis() + 1000;  // give the log line time to go out
  } else if (action == "logs") {
    wantLogReplay = true;
  } else if (action == "state") {
    stateDirty = true;
  } else if (action == "update") {
    requestOta(doc["url"] | "");
  } else if (action.length()) {
    logf('W', "Unknown action: %s", action.c_str());
  }
}

static void onMessage(char *topic, byte *payload, unsigned int len) {
  // Copy out: PubSubClient reuses this buffer for the next publish
  pendingTopic = topic;
  pendingPayload = String((const char *)payload, len);
  pendingPayload.trim();
}

void handleMessages() {
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

void setupMqtt() {
  net.setCACert(ROOT_CA);
  mqtt.setServer(conn.host.c_str(), conn.port);
  mqtt.setCallback(onMessage);
  mqtt.setKeepAlive(30);
  mqtt.setBufferSize(1024);
}

bool connectMqtt() {
  String clientId = "esp32-" + WiFi.macAddress();
  clientId.replace(":", "");

  // Last will: the broker publishes "offline" for us if we drop off without saying goodbye
  if (!mqtt.connect(clientId.c_str(), conn.user.c_str(), conn.pass.c_str(), TOPIC_STATUS.c_str(), 1, true,
                    "offline")) {
    logf('E', "MQTT connect failed (state %d)", mqtt.state());
    return false;
  }
  mqtt.publish(TOPIC_STATUS.c_str(), "online", true);
  mqtt.subscribe(TOPIC_SET.c_str(), 1);
  mqtt.subscribe(TOPIC_CMD.c_str(), 1);
  logf('I', "MQTT connected as %s", clientId.c_str());
  markFirmwareGood();
  publishState();
  return true;
}
