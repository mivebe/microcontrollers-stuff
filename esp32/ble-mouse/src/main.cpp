// ESP32 as a Bluetooth LE mouse that moves the cursor now and then, so the paired computer never
// goes idle (no screen lock, chat status stays "active"). Moves look like a hand on a mouse: a
// curved path that speeds up and slows down, sometimes a few in a row, wandering around within
// `radius` pixels of where the cursor started so it never drifts off.
//
// BOOT button: short press turns jiggling on/off. Serial (115200): type "help".
// LED: blinking = waiting for a computer to connect, on = jiggling, off = paused,
//      dark while the cursor is moving.
#include <Arduino.h>
#include <Preferences.h>
#include <esp_task_wdt.h>

#include "ble_mouse.h"

// What the computer sees. Defaults look like a generic PixArt-based optical mouse; to mimic a
// specific mouse, copy its values (see README). The computer caches all of this at pairing time:
// after changing anything here, remove the mouse in Bluetooth settings and pair it again.
const MouseIdentity IDENTITY = {
    .name = "WD Mouse",
    .manufacturer = "PixArt",
    .model = "WD Mouse",
    .firmware = "1.0.3",
    .vendorIdSource = 0x02,   // USB-IF
    .vendorId = 0x093A,       // PixArt Imaging
    .productId = 0x2510,      // PixArt optical mouse
    .version = 0x0100,
};
const uint32_t BATTERY_DRAIN_MIN = 480;     // battery drops 1% every 8-12 hours of use,
const uint32_t BATTERY_DRAIN_MAX = 720;     //   about a month from full to recharge,
const uint8_t BATTERY_RECHARGE_AT = 15;     //   and is "recharged" to 100% when it gets this low

const int LED_PIN = 2;
const int BUTTON_PIN = 0;                   // BOOT button, active low
const uint32_t WDT_TIMEOUT_S = 30;          // reboot if loop() hangs this long
const uint32_t SHORT_PRESS_MS = 1000;
const uint32_t STEP_MS = 10;                // loop period, so at most one mouse report per 10 ms
const int FOLLOW_UP_PERCENT = 35;           // chance that a move is followed by another one
const int OVERSHOOT_PERCENT = 30;           // chance that a move overshoots and corrects back

struct Settings {
  bool on = true;
  uint16_t minS = 20;       // jiggle at a random moment between minS and maxS seconds apart
  uint16_t maxS = 60;
  uint16_t radius = 40;     // the cursor wanders within this many pixels of where it started
  uint8_t battery = 100;    // simulated battery level, %
} cfg;

// One move from the current offset to a target: a cubic Bezier curve, eased so it starts and
// ends slowly, with a slight hand tremor on top. The offsets are relative to where the cursor was
// before the first move.
struct Motion {
  bool active = false;
  uint32_t startMs, durationMs;
  float x0, y0, cx1, cy1, cx2, cy2, x1, y1;
  float tremorAmp, tremorHz, tremorPhase;
  bool correct = false;     // after this move, a short correction to (fixX, fixY)
  float fixX, fixY;
} motion;

Preferences prefs;
uint32_t nextJiggleAt = 0;
uint32_t nextBatteryDropAt = 0;
uint32_t jiggleCount = 0;
int offsetX = 0, offsetY = 0;   // where the cursor is, relative to where the device started moving it

void loadSettings() {
  prefs.begin("mouse", false);  // read-write so the first boot creates it instead of logging an error
  cfg.on = prefs.getBool("on", cfg.on);
  cfg.minS = prefs.getUShort("min", cfg.minS);
  cfg.maxS = prefs.getUShort("max", cfg.maxS);
  cfg.radius = prefs.getUShort("radius", cfg.radius);
  cfg.battery = prefs.getUChar("battery", cfg.battery);
  prefs.end();
}

void saveSettings() {
  prefs.begin("mouse", false);
  prefs.putBool("on", cfg.on);
  prefs.putUShort("min", cfg.minS);
  prefs.putUShort("max", cfg.maxS);
  prefs.putUShort("radius", cfg.radius);
  prefs.putUChar("battery", cfg.battery);
  prefs.end();
}

void scheduleNext() {
  nextJiggleAt = millis() + random(cfg.minS * 1000UL, cfg.maxS * 1000UL + 1);
}

void setBattery(uint8_t level) {
  cfg.battery = level;
  mouseSetBattery(level);
  saveSettings();
  nextBatteryDropAt = millis() + random(BATTERY_DRAIN_MIN, BATTERY_DRAIN_MAX + 1) * 60000UL;
}

// Real mice report a battery that slowly runs down, not 100% forever
void drainBattery() {
  if ((int32_t)(millis() - nextBatteryDropAt) < 0) return;
  setBattery(cfg.battery <= BATTERY_RECHARGE_AT ? 100 : cfg.battery - 1);
  Serial.printf("Battery %u%%\n", cfg.battery);
}

float randf(float lo, float hi) { return lo + (hi - lo) * (esp_random() / 4294967295.0f); }

// Minimum-jerk easing: the speed profile of a real hand movement
float ease(float t) { return t * t * t * (10 + t * (-15 + t * 6)); }

// Start a curved move from the current offset to (tx, ty), bent sideways by up to `bendMax` of
// its length (both control points to the same side)
void moveTo(float tx, float ty, uint32_t durationMs, float bendMax) {
  float len = max(1.0f, hypotf(tx - offsetX, ty - offsetY));
  float nx = -(ty - offsetY) / len, ny = (tx - offsetX) / len;
  float bend = randf(-bendMax, bendMax) * len;
  motion.x0 = offsetX;
  motion.y0 = offsetY;
  motion.x1 = tx;
  motion.y1 = ty;
  motion.cx1 = offsetX + (tx - offsetX) * randf(0.2f, 0.4f) + nx * bend * randf(0.6f, 1.0f);
  motion.cy1 = offsetY + (ty - offsetY) * randf(0.2f, 0.4f) + ny * bend * randf(0.6f, 1.0f);
  motion.cx2 = offsetX + (tx - offsetX) * randf(0.6f, 0.8f) + nx * bend * randf(0.6f, 1.0f);
  motion.cy2 = offsetY + (ty - offsetY) * randf(0.6f, 0.8f) + ny * bend * randf(0.6f, 1.0f);
  motion.tremorAmp = randf(0.3f, 1.0f);   // physiological tremor: under a pixel or so, 8-12 Hz
  motion.tremorHz = randf(8, 12);
  motion.tremorPhase = randf(0, 2 * PI);
  motion.durationMs = durationMs;
  motion.startMs = millis();
  motion.active = true;
}

void startMove() {
  // Pick a target inside the radius, at least a few pixels away from where we are
  float tx, ty, len;
  do {
    float r = cfg.radius * sqrtf(randf(0, 1));
    float a = randf(0, 2 * PI);
    tx = r * cosf(a);
    ty = r * sinf(a);
    len = hypotf(tx - offsetX, ty - offsetY);
  } while (len < min(5.0f, (float)cfg.radius));

  uint32_t duration = constrain((uint32_t)((200 + len * 6) * randf(0.8f, 1.25f)), 150, 2000);
  // Hands often overshoot a little and pull back
  motion.correct = len > 15 && random(100) < OVERSHOOT_PERCENT;
  if (motion.correct) {
    float over = randf(0.04f, 0.12f);
    motion.fixX = tx;
    motion.fixY = ty;
    tx += (tx - offsetX) * over;
    ty += (ty - offsetY) * over;
  }
  moveTo(tx, ty, duration, 0.33f);
  jiggleCount++;
  Serial.printf("Move #%lu to (%d, %d), %u ms%s\n", (unsigned long)jiggleCount, (int)tx, (int)ty,
                duration, motion.correct ? ", overshoot" : "");
}

// Advance the current move: send the difference between where the path is now and where the
// cursor already is. Called every loop; returns true when the move (and any correction) finished.
bool stepMove() {
  uint32_t elapsed = millis() - motion.startMs;
  float t = min(1.0f, elapsed / (float)motion.durationMs);
  float e = ease(t), u = 1 - e;
  float x = u * u * u * motion.x0 + 3 * u * u * e * motion.cx1 + 3 * u * e * e * motion.cx2 + e * e * e * motion.x1;
  float y = u * u * u * motion.y0 + 3 * u * u * e * motion.cy1 + 3 * u * e * e * motion.cy2 + e * e * e * motion.y1;
  // Tremor across the direction of travel, fading out at both ends so the move lands on target
  float len = max(1.0f, hypotf(motion.x1 - motion.x0, motion.y1 - motion.y0));
  float wobble = motion.tremorAmp * sinf(PI * t) *
                 sinf(2 * PI * motion.tremorHz * elapsed / 1000.0f + motion.tremorPhase);
  x += -(motion.y1 - motion.y0) / len * wobble;
  y += (motion.x1 - motion.x0) / len * wobble;

  int dx = (int)lroundf(x) - offsetX;
  int dy = (int)lroundf(y) - offsetY;
  if (dx || dy) {
    mouseMove(dx, dy);
    offsetX += dx;
    offsetY += dy;
  }
  if (t < 1) return false;
  if (motion.correct) {
    motion.correct = false;
    moveTo(motion.fixX, motion.fixY, random(90, 220), 0.15f);
    return false;
  }
  motion.active = false;
  return true;
}

void printStatus() {
  Serial.printf("Name \"%s\", %s, jiggling %s, every %u-%u s, radius %u px, %lu moves so far\n",
                IDENTITY.name, mouseConnected() ? "connected" : "waiting for connection",
                cfg.on ? "on" : "off", cfg.minS, cfg.maxS, cfg.radius,
                (unsigned long)jiggleCount);
  Serial.printf("Shows as %s %04X:%04X v%04X, address %s, battery %u%%\n", IDENTITY.manufacturer,
                IDENTITY.vendorId, IDENTITY.productId, IDENTITY.version, mouseAddress().c_str(),
                cfg.battery);
  if (cfg.on && mouseConnected())
    Serial.printf("Next move in %ld s\n", (long)(nextJiggleAt - millis()) / 1000);
}

void setOn(bool on) {
  cfg.on = on;
  saveSettings();
  scheduleNext();
  Serial.printf("Jiggling %s\n", on ? "on" : "off");
}

void handleCommand(String line) {
  line.trim();
  line.toLowerCase();
  if (line.isEmpty()) return;
  int a, b;
  if (line == "on") {
    setOn(true);
  } else if (line == "off") {
    setOn(false);
  } else if (line == "status") {
    printStatus();
  } else if (line == "jiggle") {
    if (!mouseConnected()) Serial.println("Not connected");
    else if (!motion.active) startMove();
  } else if (sscanf(line.c_str(), "interval %d %d", &a, &b) == 2 && a >= 1 && b >= a && b <= 3600) {
    cfg.minS = a;
    cfg.maxS = b;
    saveSettings();
    scheduleNext();
    printStatus();
  } else if (sscanf(line.c_str(), "radius %d", &a) == 1 && a >= 1 && a <= 500) {
    cfg.radius = a;
    saveSettings();
    printStatus();
  } else if (sscanf(line.c_str(), "battery %d", &a) == 1 && a >= 1 && a <= 100) {
    setBattery(a);
    printStatus();
  } else if (line == "restart") {
    ESP.restart();
  } else {
    Serial.println("Commands:\n"
                   "  on | off              start / stop jiggling (remembered)\n"
                   "  status                show settings and connection\n"
                   "  jiggle                move once now\n"
                   "  interval <min> <max>  seconds between moves, 1-3600\n"
                   "  radius <px>           how far the cursor may wander, 1-500\n"
                   "  battery <percent>     set the simulated battery level, 1-100\n"
                   "  restart               reboot the board");
  }
}

void readSerial() {
  static String line;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      handleCommand(line);
      line = "";
    } else if (line.length() < 64) {
      line += c;
    }
  }
}

void readButton(uint32_t now) {
  static uint32_t pressedAt = 0;
  bool pressed = digitalRead(BUTTON_PIN) == LOW;
  if (pressed && !pressedAt) {
    pressedAt = now ? now : 1;
  } else if (!pressed && pressedAt) {
    uint32_t held = now - pressedAt;
    pressedAt = 0;
    if (held > 30 && held < SHORT_PRESS_MS) setOn(!cfg.on);  // > 30 ms filters contact bounce
  }
}

void renderLed(uint32_t now, bool connected) {
  bool lit;
  if (!connected) lit = now % 1000 < 150;
  else if (!cfg.on) lit = false;
  else lit = !motion.active;
  digitalWrite(LED_PIN, lit);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  esp_task_wdt_init(WDT_TIMEOUT_S, true);
  esp_task_wdt_add(NULL);

  loadSettings();
  mouseBegin(IDENTITY, cfg.battery);
  setBattery(cfg.battery);  // schedules the first drop
  Serial.printf("\nBLE mouse \"%s\" ready, pair it from the computer's Bluetooth settings\n",
                IDENTITY.name);
  printStatus();
  Serial.println("Type \"help\" for commands");
}

void loop() {
  esp_task_wdt_reset();
  uint32_t now = millis();

  static bool wasConnected = false;
  bool connected = mouseConnected();
  if (connected != wasConnected) {
    wasConnected = connected;
    motion.active = false;
    Serial.println(connected ? "Connected" : "Disconnected, waiting for the computer to reconnect");
    if (connected) scheduleNext();
  }

  if (connected && motion.active) {
    if (stepMove()) {
      // Sometimes a person nudges the mouse a few times in a row
      if (cfg.on && random(100) < FOLLOW_UP_PERCENT)
        nextJiggleAt = millis() + random(150, 1200);
      else
        scheduleNext();
    }
  } else if (connected && cfg.on && (int32_t)(now - nextJiggleAt) >= 0) {
    startMove();
  }

  if (connected) drainBattery();
  readSerial();
  readButton(now);
  renderLed(now, connected);
  delay(STEP_MS);
}
