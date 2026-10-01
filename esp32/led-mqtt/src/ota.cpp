#include "ota.h"

#include <ArduinoJson.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <esp_ota_ops.h>
#include <esp_task_wdt.h>

#include "config.h"
#include "device.h"
#include "logger.h"
#include "version.h"

static String otaUrl;  // set by requestOta(), handled in otaLoop()

// Tells the Arduino core not to confirm a fresh update at boot; we do it after MQTT connects
extern "C" bool verifyRollbackLater() { return true; }

bool firmwareOnProbation() {
  esp_ota_img_states_t st;
  return esp_ota_get_state_partition(esp_ota_get_running_partition(), &st) == ESP_OK &&
         st == ESP_OTA_IMG_PENDING_VERIFY;
}

void markFirmwareGood() {
  if (firmwareOnProbation()) {
    esp_ota_mark_app_valid_cancel_rollback();
    logf('I', "Firmware %s confirmed after update", FW_VERSION);
  }
}

void reportRollback() {
  if (esp_ota_get_last_invalid_partition())
    logf('W', "An earlier update failed to start and was rolled back");
}

static void publishOta(const char *state, int progress = -1, const char *error = nullptr) {
  JsonDocument doc;
  doc["state"] = state;
  if (progress >= 0) doc["progress"] = progress;
  if (error) doc["error"] = error;
  char out[192];
  size_t n = serializeJson(doc, out, sizeof(out));
  mqtt.publish(TOPIC_OTA.c_str(), (const uint8_t *)out, n, false);
}

static void runOta(const String &url) {
  logf('I', "Update: downloading %s", url.c_str());
  publishOta("downloading", 0);

  // Separate TLS client for the download. No certificate pinning, so a CA change on the
  // firmware host can never lock us out of updates.
  WiFiClientSecure client;
  client.setInsecure();

  static int lastPct;
  lastPct = -1;
  httpUpdate.rebootOnUpdate(false);
  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  httpUpdate.onProgress([](int done, int total) {
    esp_task_wdt_reset();  // a download takes longer than the watchdog timeout
    int pct = total > 0 ? (int64_t)done * 100 / total : 0;
    if (pct / 5 != lastPct / 5) {
      lastPct = pct;
      publishOta("downloading", pct);
    }
    mqtt.loop();  // keep the broker connection alive meanwhile
  });

  HTTPUpdateResult res = httpUpdate.update(client, url);
  if (res == HTTP_UPDATE_OK) {
    logf('I', "Update: written, rebooting into the new firmware");
    publishOta("rebooting", 100);
    restartAt = millis() + 1000;
  } else {
    String err = httpUpdate.getLastErrorString();
    logf('E', "Update failed: %s", err.c_str());
    publishOta("failed", -1, err.c_str());
  }
}

void requestOta(const String &url) {
  if (!url.startsWith("https://")) {
    logf('W', "Update needs an https:// url");
    return;
  }
  otaUrl = url;
}

void otaLoop() {
  if (otaUrl.length() && !restartAt) {
    String url = otaUrl;
    otaUrl = "";
    runOta(url);
  }
}
