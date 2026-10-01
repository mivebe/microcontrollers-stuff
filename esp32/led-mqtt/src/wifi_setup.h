// WiFi: connects to the strongest remembered network, and opens a setup portal (an open access
// point "ESP32-Setup-xxxxxx" with a captive web page) when no network is known, or none could be
// reached for 3 minutes. The known networks keep being retried while the portal is open.
#pragma once
#include <Arduino.h>

void wifiBegin();         // in setup(), after loadConnection()
void wifiLoop();          // every loop()
bool wifiPortalOpen();
void wifiFactoryReset();  // forget all networks and open the portal
