# ble-mouse

The ESP32 pretends to be a Bluetooth mouse and nudges the cursor now and then, so the computer never
goes idle: no screen lock, no sleep, chat status stays "active". Every 20–60 s (random) it moves the
cursor like a hand would: a slightly curved path that speeds up and slows down, sometimes a couple of
nudges in a row. It wanders within 40 px of where it started, so the cursor never drifts away.

Standalone: no WiFi, nothing to install on the computer. Power it from any USB port or charger.

## Pairing

1. Flash it (`python -m platformio run -t upload`) or just power it up. The LED blinks: it is waiting for a computer.
2. On the computer: **Settings → Bluetooth & devices → Add device → Bluetooth**, pick **WD Mouse**.
3. The LED stays on. It reconnects by itself after a reboot or when the computer wakes up.

To pair it with a different computer, remove it from the old one's Bluetooth list first.

## Controls

| | |
|---|---|
| BOOT button, short press | jiggling on / off (remembered across reboots) |
| LED blinking | waiting for a computer to connect |
| LED on | connected and jiggling (dark while the cursor is moving) |
| LED off | connected, jiggling paused |

Serial monitor (`python -m platformio device monitor`, 115200; close it before uploading), type a command and Enter:

| Command | |
|---|---|
| `on` / `off` | start / stop jiggling |
| `status` | connection, settings, next jiggle |
| `jiggle` | move once now (to test) |
| `interval <min> <max>` | seconds between moves, e.g. `interval 30 120` |
| `radius <px>` | how far the cursor may wander, e.g. `radius 10` for barely visible moves |
| `battery <percent>` | set the simulated battery level |
| `restart` | reboot |

Settings are kept in flash.

## Looking like a real mouse

To the computer this is an ordinary Bluetooth mouse: a "HID-compliant mouse", and its moves are
real hardware input, not injected by software. Nothing about it says ESP32 (the usual hobby library
gives itself away with vendor id `E502`, an Espressif address prefix and its own report layout, so
`src/ble_mouse.*` does the Bluetooth side itself):

| What the computer sees | Value |
|---|---|
| Name / model | WD Mouse |
| Manufacturer | PixArt, vendor/product `093A:2510` v`0100`: a generic PixArt optical mouse |
| Firmware revision | 1.0.3 |
| Bluetooth address | random static (`F5:…`), as most real BLE mice use; fixed per board |
| Report layout | Report ID 1: 5 buttons, 16-bit X/Y, wheel, horizontal wheel, like mainstream mice |
| Pairing | Secure Connections, no PIN ("Just Works") |
| Battery | starts at 100%, drops 1% every 8–12 h while connected, back to 100% at 15%, with notifications |
| Movement | curved, eased, slight 8–12 Hz hand tremor, sometimes overshoots and corrects, sometimes a few nudges in a row |

All of it is in `IDENTITY` and the constants at the top of `src/main.cpp`.

**Power it from a USB charger or power bank, not from the computer it's paired with.** Plugged into
that computer it would also show up as a "Silicon Labs CP210x" serial port, which no mouse has. Use the
computer's USB only for flashing and the serial monitor.

To copy a specific real mouse: pair it with Windows, then in **Device Manager → Mice and other pointing
devices → HID-compliant mouse → Details → Hardware Ids** read `VID&ssvvvv_PID&pppp_REV&rrrr`. `ss` is
the vendor id source; `vvvv`, `pppp` and `rrrr` have their two bytes swapped. Example: the stock library
shows up as `VID&0202E5_PID&11A1_REV&1002`, meaning source `0x02`, vendor `0xE502`, product `0xA111`,
version `0x0210`.

The computer remembers all of this from the pairing. **After changing any of it, remove the mouse from
Bluetooth settings and pair it again.**
