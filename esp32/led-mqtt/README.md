# led-mqtt

Control ESP32 boards' LEDs from anywhere through HiveMQ Cloud (MQTT). One firmware, any number of
boards in different places.

```
Phone / browser ──wss:8884──► HiveMQ Cloud ◄──mqtts:8883── ESP32 boards (any 2.4 GHz WiFi) ── LED
```

Web app: **https://mivebe.github.io/microcontrollers-stuff/** (deployed by `.github/workflows/pages.yml`
on every push that touches `web/` or the firmware, together with the firmware for OTA updates).

Plans: [docs/](docs/) (current work and future ideas, as checklists).

## Code

| File | What it does |
|---|---|
| `src/main.cpp` | `setup()`, `loop()`, watchdog, offline reboot, BOOT button |
| `src/config.*` | Everything kept in flash: broker login, device id + name, WiFi networks, LED settings |
| `src/wifi_setup.*` | Connecting to the known networks + the WiFi setup portal |
| `src/device.*` | LED output, MQTT connection, state reports, commands |
| `src/logger.*` | Log lines to Serial, a replay buffer and MQTT |
| `src/ota.*` | Firmware updates over the air with automatic rollback |
| `include/version.h` | Firmware version |
| `include/secrets.h` | Your broker login etc. (git-ignored, see `secrets.example.h`) |

## Topics

Every board has an id made from its MAC address (`esp32-a1b2c3`) and uses `<base>/<id>/…`, where
`<base>` is `TOPIC_BASE` from `secrets.h` (e.g. `mivebe`).

| Topic | Direction | Payload |
|---|---|---|
| `<base>/<id>/set` | to device | `on`, `off`, `toggle` |
| `<base>/<id>/cmd` | to device | JSON, any subset: `{"on":true,"brightness":0-100,"mode":"solid\|blink\|breathe","period":100-10000,"interval":5-3600,"name":"Garage"}` or `{"action":"restart\|logs\|state"}` or `{"action":"update","url":"https://…/firmware-x.y.z.bin"}` |
| `<base>/<id>/state` | from device, retained | `id`, `name`, settings + device info (`uptime`, `rssi`, `heap`, `ip`, `fw`, `reset`, …), on change and every `interval` s |
| `<base>/<id>/status` | from device, retained | `online` / `offline` (offline is the MQTT last will) |
| `<base>/<id>/log` | from device | `{"seq","ts","up","lvl":"I\|W\|E","msg"}`; `{"action":"logs"}` replays the last 60 lines with `"hist":true` |
| `<base>/<id>/ota` | from device | update progress: `{"state":"downloading","progress":0-100}`, `{"state":"rebooting"}`, `{"state":"failed","error"}` |

The web app finds boards by subscribing to `<base>/+/status` and `<base>/+/state` (both retained).

## Setup

1. **HiveMQ Cloud → Access Management**: create two credentials with publish + subscribe permission,
   one shared by all boards (`esp32-device`) and one for the web app (`web-app`).
2. Fill in `include/secrets.h` (git-ignored; template in `include/secrets.example.h`): the device
   credential, the topic base and, optionally, your home WiFi.
3. Upload the firmware over USB (PlatformIO **Upload**) and open the **Monitor**. The upload saves the
   broker settings to the board's flash, which OTA builds rely on.
4. Open the web app and log in with the `web-app` credential and the same topic base.

## Shipping a board to someone else (remote install)

1. At home: upload the firmware over USB (step 3 above). To ship a board that doesn't know your home
   WiFi, remove `WIFI_SSID`/`WIFI_PASS` from `secrets.h` for that upload, or factory-reset it afterwards.
2. Check it shows up in the web app, and give it a name (Device tab).
3. Send it. The owner plugs it into any USB charger. Not finding a known WiFi, it opens the setup portal
   (right away if it knows no WiFi at all, after 3 minutes if it only knows yours):
   - join the WiFi **`ESP32-Setup-xxxxxx`** from a phone; the setup page opens by itself (otherwise
     browse to `http://192.168.4.1`)
   - pick their WiFi, enter the password, optionally a name, **Save and connect**
4. The board goes online and appears in your web app. From then on: control, logs and updates are remote.

What the owner may need later:

| Situation | What happens / what they do |
|---|---|
| New router or WiFi password | After 3 min without WiFi the board opens the setup portal again by itself (and keeps retrying the old networks). They redo step 3. |
| Something is off | Hold the **BOOT** button 10 s: the board forgets all WiFi networks and opens the portal. The broker login is kept, so it stays yours to manage. |
| Board moves between places | It remembers up to 5 networks and joins the strongest one in range. |

LED signals: **double blink** = setup portal open, the board needs WiFi. Otherwise the LED shows its
normal settings. A short press on BOOT toggles the LED.

## Firmware updates over the air

1. Change the firmware and bump `FW_VERSION` in `include/version.h`.
2. Push to `master`. The Pages workflow builds the firmware **without** `secrets.h` (the repo is public, so
   the published `.bin` contains no credentials) and publishes `firmware/firmware-<version>.bin` plus
   `firmware/manifest.json` next to the web app.
3. The web app checks the manifest on load, every 30 min and on **Device → Check for updates**. Boards
   running an older version get an update badge in the device list. Nothing installs on its own: press
   **Update to vX** on a board, or **Update all**, which updates the boards one at a time and stops at
   the first failure. Tip: update your own board first and the rest a day later.
4. The board downloads the image into its spare app slot and reboots. Progress shows in the web app.

Safety net: after an update the new firmware is on probation until it connects to the broker. If it
crashes, hangs or can't connect (the 10 min offline reboot, which also runs while the setup portal is
open), the bootloader goes back to the previous version and the next boot logs "An earlier update
failed to start and was rolled back".

Builds without `secrets.h` use the broker settings saved in flash. To change the broker login, edit
`secrets.h` and upload once over USB to each board.

## Reliability

- WiFi and MQTT reconnect automatically; the board reboots after 10 min without the broker (not while
  someone may be using the setup portal).
- A 30 s watchdog reboots the board if the code hangs (shows up as "watchdog" restart reason).

## TLS

The broker's certificate chains to Let's Encrypt **ISRG Root X1** (valid until 2035), embedded in
`include/root_ca.h`. If HiveMQ ever changes CA, update that file.

## Web app development

React + TypeScript + Tailwind v4 + shadcn/ui + lucide-react, in `web/`.

```sh
cd web
npm install
npm run dev      # local dev server (also reachable from your phone on the same WiFi)
npm run build    # type-check + production build into dist/
```

Add shadcn components with `npx shadcn@latest add <name>`.
