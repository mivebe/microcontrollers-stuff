#include <Arduino.h>
#include <WiFi.h>

// Onboard blue LED on most ESP32 DevKit boards
#define LED_PIN 2

void printChipInfo() {
  Serial.println("\n=== ESP32 chip info ===");
  Serial.printf("Model:       %s rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("Cores:       %d @ %d MHz\n", ESP.getChipCores(), ESP.getCpuFreqMHz());
  Serial.printf("Flash:       %u MB\n", ESP.getFlashChipSize() / (1024 * 1024));
  Serial.printf("Free heap:   %u bytes\n", ESP.getFreeHeap());
  Serial.printf("MAC:         %s\n", WiFi.macAddress().c_str());
  Serial.printf("SDK:         %s\n", ESP.getSdkVersion());
}

void scanWifi() {
  Serial.println("\n=== WiFi scan ===");
  int n = WiFi.scanNetworks();
  Serial.printf("Found %d networks\n", n);
  for (int i = 0; i < n; i++) {
    Serial.printf("  %2d. %-32s %4d dBm  ch %d\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
  }
  WiFi.scanDelete();
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(LED_PIN, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  printChipInfo();
  scanWifi();
  Serial.println("\nBlinking LED on GPIO2...");
}

void loop() {
  static uint32_t count = 0;
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);
  Serial.printf("alive, uptime %lus, blink #%lu\n", millis() / 1000, ++count);
}
