// The device itself: LED output, WiFi + MQTT connection, state reports and commands
#pragma once
#include <PubSubClient.h>

// Onboard blue LED and BOOT button on most ESP32 DevKit boards
#define LED_PIN 2
#define BUTTON_PIN 0
#define LED_CHANNEL 0

extern PubSubClient mqtt;
extern bool stateDirty;     // publish the state on the next loop()
extern uint32_t restartAt;  // millis() at which loop() reboots, 0 = none

const char *resetReason();
void renderLed();
void publishState();
void settingsChanged();  // save to flash and report

void syncTime();
void setupMqtt();
bool connectMqtt();
void handleMessages();  // commands received during mqtt.loop()
