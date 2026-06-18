# ESP32_WifiMqttDashPlus

Firmware for the ESP32 that combines Wi-Fi connectivity, an MQTT client, and a browser-based configuration dashboard. It is intended for Smart Farm / IoT use cases where the device joins a network, connects to an MQTT broker, and receives sensor data (for example temperature and humidity) published as JSON.

## Summary

On boot, the ESP32 loads Wi-Fi and MQTT settings from flash (`Preferences`). It attempts to connect to the saved Wi-Fi network; if that fails or no credentials are stored, it starts **Access Point (AP) mode** so you can configure it from a web browser at `192.168.4.1`.

Once online, the device serves a **Dashboard Plus** web UI for viewing connection status and editing network settings. It maintains an **MQTT** connection, subscribes to a configured topic, and parses incoming JSON payloads for `tempc` and `hum` fields. The device is also reachable via **mDNS** at `http://myESP32.local`.

## Prerequisites

- **[PlatformIO](https://platformio.org/)** — VS Code extension or CLI
- **ESP32 DOIT DevKit V1** (or compatible board; `esp32doit-devkit-v1` in `platformio.ini`)
- **USB data cable** for flashing and serial monitoring
- **Wi-Fi network** (optional at first boot; AP mode can be used for initial setup)
- **MQTT broker** (for full MQTT functionality; e.g. Mosquitto on a local server or cloud broker)

### Dependencies (managed by PlatformIO)

| Library | Purpose |
|---------|---------|
| [ESPAsyncWebServer](https://github.com/mathieucarbou/ESPAsyncWebServer) | Async HTTP web server |
| [AsyncTCP](https://github.com/mathieucarbou/AsyncTCP) | TCP stack for async web server |
| [ESP-DashboardPlus](https://github.com/aaronbeckmann/ESP-DashboardPlus) | Web dashboard UI |
| [PubSubClient](https://github.com/knolleary/pubsubclient) | MQTT client |
| [ArduinoJson](https://arduinojson.org/) | JSON parsing for MQTT payloads |
| [TaskScheduler](https://github.com/arkhipenko/TaskScheduler) | Non-blocking periodic tasks |
| [ezLED](https://github.com/ArduinoGetStarted/ezLED) | Built-in status LED patterns |

## Features

- **Wi-Fi station mode** — Connects using SSID and password stored in NVS
- **AP fallback / provisioning** — Starts `ESP32-AP` at `192.168.4.1` when Wi-Fi is unavailable or unconfigured
- **Web dashboard** — Network configuration and status cards served at `/` on port 80
- **Persistent settings** — Wi-Fi and MQTT credentials saved to `Preferences` (`config` namespace)
- **MQTT client** — Auto-reconnect when disconnected; status shown on dashboard
- **JSON sensor parsing** — Extracts `tempc` and `hum` from subscribed MQTT messages
- **mDNS discovery** — Device advertised as `myESP32.local`
- **Status LED** — Built-in LED blinks on successful MQTT connection
- **Dashboard actions** — Save & Restart, Reset network settings, and Restart device
- **Scheduled tasks** — Wi-Fi status checks, MQTT connect/reconnect, dashboard loop, and LED updates run via TaskScheduler
- **Serial debug output** — Optional debug logging at 115200 baud (enable with `#define _DEBUG_` in `main.cpp`)

## Quick Start

```bash
platformio run              # Build firmware
platformio run -t upload    # Flash to ESP32
platformio run -t monitor   # Serial monitor (115200 baud)
platformio test             # Run tests
platformio run -t lint      # Lint
```

### First-time setup

1. Flash the firmware to the ESP32.
2. If the device has no saved Wi-Fi credentials, connect to the AP **`ESP32-AP`** (password defined in `src/Credentials.h`).
3. Open `http://192.168.4.1` (or `http://myESP32.local` when mDNS is available).
4. Enter Wi-Fi SSID/password and MQTT broker settings, then click **Save & Restart**.
5. After reboot, the device connects to Wi-Fi and the MQTT broker; sensor messages on the subscribed topic are parsed and logged to serial.

## Project Structure

```
src/
├── main.cpp           # Entry point: Wi-Fi, MQTT, web server, task scheduler
├── DashboardUI.h      # Dashboard cards, inputs, and action buttons
├── Credentials.h      # AP mode SSID/password (station creds stored in Preferences)
├── Debug.h            # Debug macros (_def, _de, _deln, _deVar)
└── dashboard_html.h   # Embedded web UI (auto-generated — do not edit manually)
```

## Notes

- Station Wi-Fi and MQTT credentials are stored in **Preferences**, not in source code. `Credentials.h` only defines AP fallback credentials.
- `dashboard_html.h` is regenerated on build. To change the web UI, edit the source HTML used by the ESP-DashboardPlus pre-build step (see library documentation).
- Uncomment or comment `#define _DEBUG_` in `main.cpp` to toggle serial debug output.
