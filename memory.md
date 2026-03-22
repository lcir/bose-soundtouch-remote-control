# Project Memory

## Current Baseline

- Project: `ESP32` local controller for `Bose SoundTouch SA-5`
- Goal: physical controller plus local LAN web UI, all over local `Wi‑Fi`
- Firmware stack: `C++`, `PlatformIO`, `Arduino`
- Default prototype board: `LOLIN/Wemos S2 Mini`
- Default display: generic `0.96" 128x64 I2C OLED`, assumed `SSD1306` compatible
- Default encoder: LaskaKit `LA132020`, rotary encoder with button and `RC` filter

## Core Functional Decisions

- Bose communication is local only:
  - `HTTP` on port `8090`
  - `WebSocket` on port `8080`
  - subprotocol `gabbo`
- Setup uses captive portal:
  - device AP `BoseRemote-xxxx`
  - config saved in `NVS` via `Preferences`
- Normal runtime has two control surfaces:
  - physical controls on the device
  - local web UI served from the ESP32 in LAN
- Physical control model is now:
  - rotary encoder = volume
  - `Source` button = next selectable source
  - `Standby` button = explicit `standby/off`
  - bi-color LED = `red standby/off`, `green on`
- Standby uses explicit Bose standby behavior, not blind `POWER` toggle
- Web power control uses dynamic `Wake` / `Standby`
- Source rotation now aims to include:
  - local `AUX`
  - `Bluetooth`
  - online/non-local sources exposed by Bose `/sources`
- Web source list is grouped for usability:
  - `AUX IN 1/2/3`
  - `Bluetooth`
  - `Online`
- `QPlay` placeholders are intentionally hidden from the UI

## Current Pinout

- `GPIO33` -> OLED `SDA`
- `GPIO35` -> OLED `SCL`
- `GPIO7` -> encoder `A`
- `GPIO9` -> encoder `B`
- `GPIO5` -> `Source` button
- `GPIO11` -> `Standby` button / setup hold at boot
- `GPIO16` -> LED `Red`
- `GPIO18` -> LED `Green`

## Hardware Assumptions

- Buttons are wired to `GND`
- Firmware uses `INPUT_PULLUP`
- OLED is powered from `3.3V`
- LED logic currently assumes `common cathode` / `active HIGH`
- If LED is `common anode`, change `POWER_LED_ACTIVE_HIGH` in [PinConfig.h](/Users/peny/Development/Projects/boser-remote-control/include/PinConfig.h)
- Add one resistor per LED branch, typically `220R` to `1k`

## Implemented Modules

- [src/BoseClient.cpp](/Users/peny/Development/Projects/boser-remote-control/src/BoseClient.cpp)
  - Bose HTTP/WebSocket communication
  - source refresh, volume refresh, now playing refresh
  - explicit `standby()`
- [src/CaptivePortal.cpp](/Users/peny/Development/Projects/boser-remote-control/src/CaptivePortal.cpp)
  - first-boot and recovery setup portal
- [src/ControlWebServer.cpp](/Users/peny/Development/Projects/boser-remote-control/src/ControlWebServer.cpp)
  - local LAN control page
  - `/api/state`, `/api/power`, `/api/source/next`, `/api/source/select`, `/api/standby`, `/api/volume`
- [src/InputController.cpp](/Users/peny/Development/Projects/boser-remote-control/src/InputController.cpp)
  - button debounce
  - encoder handling
- [src/UiRenderer.cpp](/Users/peny/Development/Projects/boser-remote-control/src/UiRenderer.cpp)
  - OLED status rendering

## Important Runtime Behavior

- Web UI starts only after station `Wi‑Fi` is connected
- Web UI stops when `Wi‑Fi` disconnects or device enters setup mode
- OLED uses explicit `I2C` config:
  - address `0x3C`
  - bus speed `400 kHz`
- Volume changes are coalesced with short delay before Bose request
- WebSocket updates do not directly mutate state from payload; they trigger refreshes
- If WebSocket is down, app falls back to periodic polling

## Known Unknowns / Unverified

- Build has not been verified locally in this environment because `PlatformIO` is not installed here
- Real hardware behavior on the actual `SA-5` is not yet tested
- OLED controller is assumed compatible with `SSD1306`; if not, `U8g2` display class may need to change
- `standby` endpoint behavior should be confirmed against the real `SA-5`
- Actual source identifiers from `/sources` should be verified on the real amplifier
- Wake via `POWER` key should be confirmed on the real `SA-5`
- Which online sources are exposed as selectable depends on the real `/sources` payload and Bose account state

## Best Next Improvements

- Add a hardware self-test mode:
  - verify OLED
  - blink both LED channels
  - test buttons
  - test encoder direction
- Add serial debug logging with controllable verbosity
- Add lightweight I2C scan mode for display bring-up
- Add web endpoint for configuration/status details if needed
- Add web-based config entry or config edit in normal mode if desired
- Confirm real Bose power-state mapping and source list behavior

## Repo Notes

- This workspace is currently not a git repository
- Main user-facing docs are:
  - [README.md](/Users/peny/Development/Projects/boser-remote-control/README.md)
  - [docs/README.md](/Users/peny/Development/Projects/boser-remote-control/docs/README.md)
  - [plan.md](/Users/peny/Development/Projects/boser-remote-control/plan.md)
