# led-mqtt

Control the ESP32's LED from anywhere through HiveMQ Cloud (MQTT).

```
Phone / browser ──wss:8884──► HiveMQ Cloud ◄──mqtts:8883── ESP32 (home 2.4 GHz WiFi) ── LED
```

## Topics

| Topic | Direction | Payload |
|---|---|---|
| `<prefix>/set` | to device | `on`, `off`, `toggle` |
| `<prefix>/state` | from device, retained | `{"on":true,"uptime":123,"rssi":-60,"heap":200000}` (on change + every 30 s) |
| `<prefix>/status` | from device, retained | `online` / `offline` (offline is the MQTT last will) |

Default prefix: `mivebe/esp32-1`.

## Setup

1. **HiveMQ Cloud → Access Management**: create two credentials with publish + subscribe permission,
   one for the device (e.g. `esp32-device`) and one for the web app (e.g. `web-app`).
2. Fill in `include/secrets.h` (git-ignored; template in `include/secrets.example.h`):
   2.4 GHz WiFi name/password and the device credential.
3. Upload the firmware (PlatformIO **Upload**) and open the **Monitor** — you should see WiFi, time sync
   and `Connecting to MQTT ... ok`.
4. Web app: `cd web && npm run dev`, open the printed URL, enter the web-app credential.

## Reliability

- WiFi and MQTT reconnect automatically; the board reboots after 10 min without a connection.
- A 30 s watchdog reboots the board if the code hangs.
- The LED state is not saved across reboots (it starts OFF).

## TLS

The broker's certificate chains to Let's Encrypt **ISRG Root X1** (valid until 2035), embedded in
`include/root_ca.h`. If HiveMQ ever changes CA, update that file.
