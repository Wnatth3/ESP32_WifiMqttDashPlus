# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Development Workflow

- **Build**: `platformio run` – compiles the firmware for the ESP32.
- **Upload**: `platformio run --target upload` – flashes the compiled binary to the board.
- **Monitor**: `platformio run --target monitor` – opens a serial monitor for real-time output.
- **Test**: `platformio test` – executes unit/integration tests in the `test/` directory.
- **Lint**: `platformio run -t lint` – runs code-style checks based on the project's linter config.

## Project Structure

```
/ (repository root)
├─ .pio/                     # PlatformIO internal files (dependencies, caches)
├─ src/                      # Core application source
│   ├─ main.cpp              # Entry point
│   ├─ Credentials.h         # Wi-Fi / MQTT credentials
│   ├─ Debug.h               # Debug macros
│   └─ dashboard_html.h      # Embedded web UI payload
├─ include/                  # Header files and README
│   └─ README                # Brief project description
├─ test/                     # Test suite
│   └─ README                # Test README
└─ platformio.ini            # PlatformIO build configuration
```

## Architecture Overview

- **ESP32 core** (`board = esp32doit-devkit-v1`) using the Arduino framework.
- **Web UI** served from `dashboard_html.h` (embedded HTML).
- **Networking** handled by `AsyncTCP`, `ESPAsyncWebServer`, and `PubSubClient` for MQTT.
- **JSON handling** via `ArduinoJson` (v7.4.3).
- **Dashboard Plus** library (`ESP-DashboardPlus`) provides UI components.

All dependencies are declared in `platformio.ini` under `lib_deps`.

## Common Commands Summary

| Action   | Command                              |
|----------|--------------------------------------|
| Build    | `platformio run`                     |
| Upload   | `platformio run --target upload`     |
| Monitor  | `platformio run --target monitor`    |
| Test     | `platformio test`                    |
| Lint     | `platformio run -t lint`             |

## Notes for Future Instances

- Dependencies are managed by PlatformIO; do not manually edit `package.json` or `requirements.txt`.
- Keep `.pio` cached dependencies; they are ignored by Git.
- The embedded UI is stored as a C header (`dashboard_html.h`).
- Credentials are kept in `Credentials.h` – never commit real secrets.
- Tests reside in `test/`; run with `platformio test` to verify changes.
