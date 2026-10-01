# led-mqtt

Control the ESP32's LED from anywhere through HiveMQ Cloud (MQTT).

```
Phone / browser ──wss:8884──► HiveMQ Cloud ◄──mqtts:8883── ESP32 (home 2.4 GHz WiFi) ── LED
```

Web app: **https://mivebe.github.io/microcontrollers-stuff/** (deployed by `.github/workflows/pages.yml`
on every push that touches `web/`).

## Topics

| Topic | Direction | Payload |
|---|---|---|
| `<prefix>/set` | to device | `on`, `off`, `toggle` |
| `<prefix>/cmd` | to device | JSON, any subset: `{"on":true,"brightness":0-100,"mode":"solid\|blink\|breathe","period":100-10000,"interval":5-3600}` or `{"action":"restart\|logs\|state"}` |
| `<prefix>/state` | from device, retained | settings + device info (`uptime`, `rssi`, `heap`, `ip`, `fw`, `reset`, …), on change and every `interval` s |
| `<prefix>/status` | from device, retained | `online` / `offline` (offline is the MQTT last will) |
| `<prefix>/log` | from device | `{"seq","ts","up","lvl":"I\|W\|E","msg"}`; `{"action":"logs"}` replays the last 60 lines with `"hist":true` |

Default prefix: `mivebe/esp32-1`. LED settings are saved in flash and survive reboots.

## Setup

1. **HiveMQ Cloud → Access Management**: create two credentials with publish + subscribe permission,
   one for the device (`esp32-device`) and one for the web app (`web-app`).
2. Fill in `include/secrets.h` (git-ignored; template in `include/secrets.example.h`):
   2.4 GHz WiFi name/password and the device credential.
3. Upload the firmware (PlatformIO **Upload**) and open the **Monitor**.
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
