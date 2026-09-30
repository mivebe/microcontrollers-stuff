#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <esp_task_wdt.h>
#include <time.h>

#include "secrets.h"
#include "root_ca.h"

// Onboard blue LED and BOOT button on most ESP32 DevKit boards
#define LED_PIN 2
#define BUTTON_PIN 0

// MQTT topics (TOPIC_PREFIX comes from secrets.h)
//   <prefix>/set     <- commands: "on", "off", "toggle"
//   <prefix>/state   -> retained JSON: {"on":true,"uptime":123,"rssi":-60,"heap":200000}
//   <prefix>/status  -> retained "online" / "offline" (offline is sent by the broker as our last will)
const String TOPIC_SET = String(TOPIC_PREFIX) + "/set";
const String TOPIC_STATE = String(TOPIC_PREFIX) + "/state";
const String TOPIC_STATUS = String(TOPIC_PREFIX) + "/status";

const uint32_t WDT_TIMEOUT_S = 30;                   // reboot if loop() hangs this long
const uint32_t STATE_INTERVAL_MS = 30 * 1000;        // periodic state heartbeat
const uint32_t OFFLINE_REBOOT_MS = 10 * 60 * 1000;   // reboot if disconnected this long

WiFiClientSecure net;
PubSubClient mqtt(net);
bool ledOn = false;
bool stateDirty = true;
uint32_t lastConnectedMs = 0;

void setLed(bool on) {
  ledOn = on;
  digitalWrite(LED_PIN, on ? HIGH : LOW);
  stateDirty = true;
  Serial.printf("LED %s\n", on ? "ON" : "OFF");
}

void publishState() {
  char json[128];
  snprintf(json, sizeof(json), "{\"on\":%s,\"uptime\":%lu,\"rssi\":%d,\"heap\":%u}",
           ledOn ? "true" : "false", millis() / 1000, WiFi.RSSI(), ESP.getFreeHeap());
  mqtt.publish(TOPIC_STATE.c_str(), json, true);
  stateDirty = false;
}

void onMessage(char *topic, byte *payload, unsigned int len) {
  String msg;
  for (unsigned int i = 0; i < len; i++) msg += (char)payload[i];
  msg.trim();
  msg.toLowerCase();
  Serial.printf("MQTT %s: %s\n", topic, msg.c_str());

  if (msg == "on") setLed(true);
  else if (msg == "off") setLed(false);
  else if (msg == "toggle") setLed(!ledOn);
}

// TLS certificate checks need a correct clock
void syncTime() {
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  Serial.print("Syncing time");
  for (int i = 0; i < 20 && time(nullptr) < 1700000000; i++) {
    delay(500);
    esp_task_wdt_reset();
    Serial.print(".");
  }
  Serial.println(time(nullptr) > 1700000000 ? " ok" : " failed (will retry TLS anyway)");
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to WiFi \"%s\"", WIFI_SSID);
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    esp_task_wdt_reset();
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf(" ok, IP %s, RSSI %d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
  } else {
    Serial.println(" failed (will keep retrying)");
  }
}

bool connectMqtt() {
  String clientId = "esp32-" + WiFi.macAddress();
  clientId.replace(":", "");
  Serial.printf("Connecting to MQTT %s:%d as %s... ", MQTT_HOST, MQTT_PORT, clientId.c_str());

  // Last will: the broker publishes "offline" for us if we drop off without saying goodbye
  bool ok = mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS,
                         TOPIC_STATUS.c_str(), 1, true, "offline");
  if (!ok) {
    Serial.printf("failed, state %d\n", mqtt.state());
    return false;
  }
  Serial.println("ok");
  mqtt.publish(TOPIC_STATUS.c_str(), "online", true);
  mqtt.subscribe(TOPIC_SET.c_str(), 1);
  publishState();
  return true;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  setLed(false);

  esp_task_wdt_init(WDT_TIMEOUT_S, true);
  esp_task_wdt_add(NULL);

  connectWifi();
  if (WiFi.status() == WL_CONNECTED) syncTime();

  net.setCACert(ROOT_CA);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setKeepAlive(30);
  lastConnectedMs = millis();
}

void loop() {
  esp_task_wdt_reset();
  uint32_t now = millis();

  if (WiFi.status() == WL_CONNECTED && mqtt.connected()) {
    lastConnectedMs = now;
    mqtt.loop();
  } else if (WiFi.status() == WL_CONNECTED) {
    // Retry MQTT every 5 s (connect() blocks for a few seconds on failure)
    static uint32_t lastTry = 0;
    if (now - lastTry > 5000 || lastTry == 0) {
      lastTry = now;
      if (time(nullptr) < 1700000000) syncTime();
      connectMqtt();
    }
  }

  // Safety net: if we've been offline for a long time, start over from scratch
  if (now - lastConnectedMs > OFFLINE_REBOOT_MS) {
    Serial.println("Offline too long, rebooting");
    ESP.restart();
  }

  // BOOT button toggles the LED locally too (active low, simple debounce)
  static bool lastPressed = false;
  bool pressed = digitalRead(BUTTON_PIN) == LOW;
  if (pressed && !lastPressed) setLed(!ledOn);
  lastPressed = pressed;

  static uint32_t lastState = 0;
  if (mqtt.connected() && (stateDirty || now - lastState > STATE_INTERVAL_MS)) {
    lastState = now;
    publishState();
  }

  delay(20);
}
