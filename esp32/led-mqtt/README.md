# led-mqtt

Control the ESP32's LED from anywhere through HiveMQ Cloud (MQTT).

```
Phone / browser ──wss:8884──► HiveMQ Cloud ◄──mqtts:8883── ESP32 (home 2.4 GHz WiFi) ── LED
```

Web app: **https://mivebe.github.io/microcontrollers-stuff/** (deployed by `.github/workflows/pages.yml`
on every push that touches `web/` or the firmware, together with the firmware for OTA updates).

## Topics

| Topic | Direction | Payload |
|---|---|---|
| `<prefix>/set` | to device | `on`, `off`, `toggle` |
| `<prefix>/cmd` | to device | JSON, any subset: `{"on":true,"brightness":0-100,"mode":"solid\|blink\|breathe","period":100-10000,"interval":5-3600}` or `{"action":"restart\|logs\|state"}` or `{"action":"update","url":"https://…/firmware-x.y.z.bin"}` |
| `<prefix>/state` | from device, retained | settings + device info (`uptime`, `rssi`, `heap`, `ip`, `fw`, `reset`, …), on change and every `interval` s |
| `<prefix>/status` | from device, retained | `online` / `offline` (offline is the MQTT last will) |
| `<prefix>/log` | from device | `{"seq","ts","up","lvl":"I\|W\|E","msg"}`; `{"action":"logs"}` replays the last 60 lines with `"hist":true` |
| `<prefix>/ota` | from device | update progress: `{"state":"downloading","progress":0-100}`, `{"state":"rebooting"}`, `{"state":"failed","error"}` |

Default prefix: `mivebe/esp32-1`. LED settings are saved in flash and survive reboots.

## Setup

1. **HiveMQ Cloud → Access Management**: create two credentials with publish + subscribe permission,
   one for the device (`esp32-device`) and one for the web app (`web-app`).
2. Fill in `include/secrets.h` (git-ignored; template in `include/secrets.example.h`):
   2.4 GHz WiFi name/password and the device credential.
3. Upload the firmware (PlatformIO **Upload**) and open the **Monitor**. This USB upload also saves the
   WiFi/broker settings to the ESP32's flash, which OTA builds rely on (see below).
4. Open the web app and log in with the `web-app` credential.

## Web app development

React + TypeScript + Tailwind v4 + shadcn/ui + lucide-react, in `web/`.

```sh
cd web
npm install
npm run dev      # local dev server (also reachable from your phone on the same WiFi)
npm run build    # type-check + production build into dist/
```

Add shadcn components with `npx shadcn@latest add <name>`.

## Reliability

- WiFi and MQTT reconnect automatically; the board reboots after 10 min without a connection.
- A 30 s watchdog reboots the board if the code hangs (shows up as "watchdog" restart reason).

## TLS

The broker's certificate chains to Let's Encrypt **ISRG Root X1** (valid until 2035), embedded in
`include/root_ca.h`. If HiveMQ ever changes CA, update that file.

## Firmware updates over the air

1. Change the firmware and bump `FW_VERSION` in `include/version.h`.
2. Push to `master`. The Pages workflow builds the firmware **without** `secrets.h` (the repo is public, so
   the published `.bin` contains no credentials) and publishes `firmware/firmware-<version>.bin` plus
   `firmware/manifest.json` next to the web app.
3. The web app checks the manifest on load, every 30 min and on **Device → Check for updates**. When it is
   newer than the version the device reports, you see an "Update available" badge. Nothing installs on
   its own: press **Update to vX** and confirm.
4. The device downloads the image into its spare app slot and reboots. Progress shows in the web app.

Safety net: after an update the new firmware is on probation until it connects to the broker. If it
crashes, hangs or can't connect (the 10 min offline reboot), the bootloader goes back to the previous
version and the next boot logs "An earlier update failed to start and was rolled back".

Builds without `secrets.h` read WiFi/broker settings from flash. To change WiFi or broker credentials,
edit `secrets.h` and upload once over USB.
