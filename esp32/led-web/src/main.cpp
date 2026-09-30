#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>

// Onboard blue LED and BOOT button on most ESP32 DevKit boards
#define LED_PIN 2
#define BUTTON_PIN 0

const char *AP_SSID = "ESP32-mivebe";
const char *AP_PASS = "esp32test";

WebServer server(80);
DNSServer dns;  // answers every hostname with our IP -> phone shows "sign in to network"
bool ledOn = false;

void setLed(bool on) {
  ledOn = on;
  digitalWrite(LED_PIN, on ? HIGH : LOW);
  Serial.printf("LED %s\n", on ? "ON" : "OFF");
}

// ---- JSON API used by the React app (web/src/App.jsx) ----

void sendLedStatus() {
  char json[96];
  snprintf(json, sizeof(json), "{\"on\":%s,\"uptime\":%lu,\"heap\":%u}",
           ledOn ? "true" : "false", millis() / 1000, ESP.getFreeHeap());
  server.send(200, "application/json", json);
}

void handleLedPost() {
  // Body is {"on":true} or {"on":false}
  setLed(server.arg("plain").indexOf("true") >= 0);
  sendLedStatus();
}

// ---- Static files from LittleFS (built from web/ into data/) ----

String contentType(const String &path) {
  if (path.endsWith(".html")) return "text/html";
  if (path.endsWith(".js")) return "application/javascript";
  if (path.endsWith(".css")) return "text/css";
  if (path.endsWith(".svg")) return "image/svg+xml";
  if (path.endsWith(".png")) return "image/png";
  if (path.endsWith(".ico")) return "image/x-icon";
  if (path.endsWith(".json")) return "application/json";
  return "text/plain";
}

// Serves "path" or its gzipped "path.gz" (the build step gzips everything)
bool serveFile(String path) {
  if (path.endsWith("/")) path += "index.html";
  String type = contentType(path);
  String fsPath = LittleFS.exists(path + ".gz") ? path + ".gz" : path;
  if (!LittleFS.exists(fsPath)) return false;

  File f = LittleFS.open(fsPath, "r");
  server.streamFile(f, type);  // adds Content-Encoding: gzip for .gz files
  f.close();
  return true;
}

void handleNotFound() {
  Serial.printf("HTTP %s from %s\n", server.uri().c_str(), server.client().remoteIP().toString().c_str());
  if (serveFile(server.uri())) return;
  // Captive portal: anything unknown (phone connectivity checks etc.) goes to the app
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/");
  server.send(302);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  setLed(false);

  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed - did you run 'Upload Filesystem Image'?");
  }

  WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t) { Serial.println("Client connected to AP"); },
               ARDUINO_EVENT_WIFI_AP_STACONNECTED);
  WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info) {
    Serial.printf("Client got IP " IPSTR "\n", IP2STR(&info.wifi_ap_staipassigned.ip));
  }, ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  dns.start(53, "*", WiFi.softAPIP());
  Serial.printf("\nWiFi AP \"%s\" (password \"%s\") -> open http://%s\n",
                AP_SSID, AP_PASS, WiFi.softAPIP().toString().c_str());

  server.on("/api/led", HTTP_GET, sendLedStatus);
  server.on("/api/led", HTTP_POST, handleLedPost);
  server.onNotFound(handleNotFound);  // static files + captive portal redirect
  server.begin();
}

void loop() {
  dns.processNextRequest();
  server.handleClient();

  // BOOT button toggles the LED (active low, simple debounce)
  static bool lastPressed = false;
  bool pressed = digitalRead(BUTTON_PIN) == LOW;
  if (pressed && !lastPressed) setLed(!ledOn);
  lastPressed = pressed;
  delay(20);
}
