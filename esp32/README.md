# ESP32

Board: ESP32-WROOM-32 DevKit (ESP32-D0WD-V3, 4 MB flash, CP2102 USB-serial).
Toolchain: [PlatformIO](https://platformio.org/) with the Arduino framework.

## Setup (Windows)

1. CP210x USB-serial driver — the board shows up as "Silicon Labs CP210x USB to UART Bridge (COMx)".
2. `python -m pip install --user platformio`
3. Optional: install the **PlatformIO IDE** VS Code extension for build/upload/monitor buttons.

## Usage

From a project folder (e.g. `hello-esp32/`):

```sh
python -m platformio run                          # build
python -m platformio run -t upload                # build + flash
python -m platformio device monitor               # serial monitor (Ctrl+C to exit)
python -m platformio device list                  # find the COM port
```

If upload picks the wrong port, add `--upload-port COM5` (or set `upload_port` in `platformio.ini`).
If upload hangs on "Connecting...", hold the **BOOT** button until writing starts.

## Projects

- `hello-esp32/` — smoke test: prints chip info, scans WiFi, blinks the onboard LED (GPIO2).
- `led-web/` — ESP32 as its own WiFi access point serving a React page from LittleFS (local control).
- `led-mqtt/` — LED controlled from anywhere via HiveMQ Cloud MQTT + React web app.
