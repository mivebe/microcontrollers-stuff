// Logging: Serial + ring buffer (replayed on request) + live MQTT on <prefix>/log
#pragma once
#include <Arduino.h>

uint64_t epochMs();                         // wall-clock ms, 0 if the clock isn't synced yet
void logf(char lvl, const char *fmt, ...);  // lvl: 'I' info, 'W' warning, 'E' error
void replayLogs();                          // republish the buffered lines with "hist":true
