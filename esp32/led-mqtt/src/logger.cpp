#include "logger.h"

#include <ArduinoJson.h>
#include <sys/time.h>

#include "config.h"
#include "device.h"

struct LogLine {
  uint32_t seq;
  uint64_t ts;  // epoch ms, 0 if the clock wasn't synced yet
  uint32_t up;  // ms since boot
  char lvl;     // 'I' info, 'W' warning, 'E' error
  char msg[112];
};

static const int LOG_CAPACITY = 60;
static LogLine logBuf[LOG_CAPACITY];
static uint32_t logSeq = 0;  // total lines ever logged; logBuf holds the last LOG_CAPACITY

uint64_t epochMs() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  if (tv.tv_sec < 1700000000) return 0;
  return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void publishLog(const LogLine &l, bool hist) {
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
