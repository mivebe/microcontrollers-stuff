// ESP32 LED remote control over MQTT. The pieces:
//   config.*      broker login, identity, WiFi networks and LED settings kept in flash
//   wifi_setup.*  connecting to known networks + the WiFi setup portal
//   device.*      LED output, MQTT connection, state reports, commands
//   logger.*      log lines to Serial, a replay buffer and MQTT
//   ota.*         firmware updates over the air with automatic rollback
#include <Arduino.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <time.h>

#include "config.h"
#include "device.h"
#include "logger.h"
#include "ota.h"
#include "version.h"
#include "wifi_setup.h"

const uint32_t WDT_TIMEOUT_S = 30;                   // reboot if loop() hangs this long
const uint32_t OFFLINE_REBOOT_MS = 10 * 60 * 1000;   // reboot if disconnected this long
const uint32_t FACTORY_RESET_MS = 10 * 1000;         // hold BOOT this long to forget all WiFi networks
const uint32_t SHORT_PRESS_MS = 1000;                // shorter presses toggle the LED

uint32_t lastConnectedMs = 0;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  ledcSetup(LED_CHANNEL, 5000, 8);
  ledcAttachPin(LED_PIN, LED_CHANNEL);

  esp_task_wdt_init(WDT_TIMEOUT_S, true);
  esp_task_wdt_add(NULL);

  loadSettings();
  loadConnection();
  logf('I', "Boot, firmware %s, device %s%s%s, reset reason: %s", FW_VERSION, conn.id.c_str(),
       conn.name.length() ? " " : "", conn.name.c_str(), resetReason());
  reportRollback();
  if (conn.host.isEmpty())
    logf('E', "No broker settings in flash: upload once over USB with include/secrets.h");
  logf('I', "Settings: LED %s, %d%%, %s, %u ms", cfg.on ? "on" : "off", cfg.brightness,
       MODE_NAMES[cfg.mode], cfg.periodMs);

  wifiBegin();
  if (WiFi.status() == WL_CONNECTED) syncTime();

  setupMqtt();
  lastConnectedMs = millis();
}

void loop() {
  esp_task_wdt_reset();
  uint32_t now = millis();

  wifiLoop();

  // Log WiFi transitions
  static bool wifiWasUp = false;
  bool wifiUp = WiFi.status() == WL_CONNECTED;
  if (wifiUp != wifiWasUp) {
    if (wifiUp) {
      logf('I', "WiFi \"%s\" connected, IP %s, RSSI %d dBm", WiFi.SSID().c_str(),
           WiFi.localIP().toString().c_str(), WiFi.RSSI());
    } else {
      logf('W', "WiFi disconnected");
    }
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

  handleMessages();
  otaLoop();
  if (restartAt && now > restartAt) {
    mqtt.publish(TOPIC_STATUS.c_str(), "offline", true);
    mqtt.disconnect();
    delay(100);
    ESP.restart();
  }

  // Safety net: if we've been offline for a long time, start over from scratch. Not while the
  // setup portal is open (that would interrupt someone entering their WiFi), except for fresh
  // firmware on probation: its reboot is what triggers the rollback.
  if (now - lastConnectedMs > OFFLINE_REBOOT_MS && (!wifiPortalOpen() || firmwareOnProbation())) {
    logf('E', "Offline too long, rebooting");
    ESP.restart();
  }

  // BOOT button (active low): short press toggles the LED, holding it 10 s is a factory reset
  static uint32_t pressedAt = 0;
  static bool resetDone = false;
  bool pressed = digitalRead(BUTTON_PIN) == LOW;
  if (pressed && !pressedAt) {
    pressedAt = now ? now : 1;
    resetDone = false;
  } else if (pressed && !resetDone && now - pressedAt >= FACTORY_RESET_MS) {
    resetDone = true;
    wifiFactoryReset();
  } else if (!pressed && pressedAt) {
    uint32_t held = now - pressedAt;
    pressedAt = 0;
    if (held > 30 && held < SHORT_PRESS_MS) {  // > 30 ms filters contact bounce
      cfg.on = !cfg.on;
      logf('I', "BOOT button: LED %s", cfg.on ? "on" : "off");
      settingsChanged();
    }
  }

  static uint32_t lastState = 0;
  if (mqtt.connected() && (stateDirty || now - lastState > cfg.reportS * 1000UL)) {
    lastState = now;
    publishState();
  }

  renderLed();
  delay(10);
}
