# AGENTS.md — ESP32_WifiMqttDashPlus

## Quick Start
```bash
platformio run           # Build
platformio run -t upload # Flash to ESP32
platformio run -t monitor # Serial monitor (115200 baud)
platformio test          # Run tests
platformio run -t lint   # Lint
```

## Project Summary
- **Target**: ESP32 DOIT DevKit V1 (`board = esp32doit-devkit-v1`)
- **Framework**: Arduino (PlatformIO `espressif32` platform)
- **Core libs**: `ESPAsyncWebServer`, `AsyncTCP`, `PubSubClient` (MQTT), `ArduinoJson` v7.4.3, `ESP-DashboardPlus`, `TaskScheduler`
- **Web UI**: Embedded in `src/dashboard_html.h` (auto-generated from `extras/dashboard.html` via pre-build script — **do not edit manually**)

## Source Layout
```
src/
├── main.cpp           # Entry point: WiFi, MQTT, web server, dashboard, task scheduler
├── Credentials.h      # AP fallback creds (AP SSID/Password); station creds stored in Preferences
├── Debug.h            # Debug macros (_def, _de, _deln, _deVar) — enabled via `#define _DEBUG_`
└── dashboard_html.h   # Compressed HTML (PROGMEM) — generated, DO NOT EDIT
```

## Key Architecture Details
- **Preferences** (`config` namespace) stores: WiFi SSID/pass, MQTT broker/port/user/pass
- **Boot flow**: Tries saved WiFi creds → if fails, starts AP mode (`192.168.4.1`)
- **Dashboard**: Served at `/` via `AsyncWebServer`; config saved via "Save & Restart" button → writes to Preferences → `ESP.restart()`
- **MQTT**: Managed via `TaskScheduler` (reconnect logic with exponential backoff: 2s → 5min after 5 failures)
- **Debug**: Define `_DEBUG_` in `main.cpp:10` to enable serial output

## Non-Obvious Gotchas
- `dashboard_html.h` is regenerated on build — changes to HTML require editing `extras/dashboard.html` (not in repo; see library docs)
- Credentials in `Credentials.h` are **only AP fallback**; real station/MQTT creds come from Preferences at runtime
- `.pio/` is gitignored — PlatformIO manages all deps via `platformio.ini`
- No `package.json`/`requirements.txt` — pure PlatformIO project
- Monitor uses `esp32_exception_decoder` filter for crash traces

## Testing
- Tests in `test/` — run with `platformio test` (no framework specified; likely Unity/ArduinoUnit)