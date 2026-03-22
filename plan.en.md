# Plan: Bose SoundTouch SA-5 ESP32 Remote

## Goal

Build a standalone physical controller for the `Bose SoundTouch SA-5` that uses an `ESP32` to control the amplifier over `Wi-Fi`, without needing a phone or the Bose app.

## Chosen Direction

- firmware will be written in `C++`
- build and project management will use `PlatformIO`
- the framework will be `Arduino` for `ESP32`
- the default prototype board will be `LOLIN/Wemos S2 Mini`
- the device will control one specific `Bose SoundTouch SA-5`
- Bose will be connected only over local `Wi-Fi`

## Hardware v1

- `LOLIN/Wemos S2 Mini`
- `OLED SSD1306 128x64` over `I2C`
- LaskaKit rotary encoder with button and `RC` filter (`LA132020`) for the first prototype
- `Source` button
- `Standby` button
- two-color `red/green` LED for status indication

## Controls

- the encoder changes volume
- volume changes are coalesced and sent after a short delay
- `Source` switches to the next logical source
- the physical `Standby` button powers the `SoundTouch` down
- device state is shown on the two-color LED
- the same basic controls are also available through the local web UI
- the web UI has a dynamic `Wake/Standby` button
- the web UI groups sources into clear items:
  - `AUX IN 1`
  - `AUX IN 2`
  - `AUX IN 3`
  - `Bluetooth`
  - `Online`

## UI

The `OLED` display should show:

- `Wi-Fi` status
- `WebSocket` connection status
- current source
- volume
- `mute` status
- short `artist/track` text when available

When source or volume changes, a highlighted overlay is shown briefly.

LED logic:

- `red`: `standby/off`
- `green`: `on`

## Setup Flow

- on first start or when configuration is missing, the device enters setup mode
- the `ESP32` creates its own `AP`
- the user connects to the captive portal
- `Wi-Fi SSID`, password, and `Bose host/IP` are saved
- configuration is stored in `NVS`
- holding the `Standby` button during boot returns the device to setup mode
- in normal mode the `ESP32` exposes a simple control web UI on the LAN

## Bose Communication

Use the local `SoundTouch` API:

- `HTTP` on port `8090`
- `WebSocket` on port `8080`
- subprotocol `gabbo`

Used endpoints:

- `/info`
- `/sources`
- `/volume`
- `/now_playing`
- `/select`
- `/key`

Rules:

- load sources dynamically from `/sources`
- the user-facing UI groups raw Bose sources into logical choices
- hide `QPlay` and technical placeholders in the UI
- `Bluetooth` and `Online` may remain selectable even outside `READY` state when that makes sense according to Bose sources
- do not parse websocket changes directly into state, but trigger refresh through `HTTP`
- when the websocket fails, switch to polling mode
- use explicit `standby/off` for power-down, not a blind `POWER` toggle
- for wake, try `POWER` and fall back to selecting a suitable source if needed
- the local web UI should use the same internal actions as the physical controls

## Electrical Wiring v1

- `GPIO33` -> OLED `SDA`
- `GPIO35` -> OLED `SCL`
- `GPIO7` -> encoder `A`
- `GPIO9` -> encoder `B`
- `GPIO5` -> `Source` button
- `GPIO11` -> `Standby` button
- `GPIO16` -> LED `Red`
- `GPIO18` -> LED `Green`

Assumptions:

- buttons are wired to `GND`
- firmware uses `INPUT_PULLUP`
- the `OLED` is powered from `3.3 V`
- Bose is not connected by wire, only over `Wi-Fi`

## v1 Limitations

- no automatic Bose device discovery on the network
- no Bose cloud presets or services
- no physical power disconnection of the amplifier
- no modification of the `SA-5` electronics

## Current Implementation State

The repository already contains an initial implementation:

- `PlatformIO` project
- captive portal
- local control web UI
- web `Wake/Standby`
- grouped `AUX/Bluetooth/Online` sources
- Bose `HTTP` and `WebSocket` client
- OLED renderer
- input layer for encoder and buttons
- basic documentation in `docs/`

## Next Practical Steps

1. Verify the build on `LOLIN/Wemos S2 Mini` using `PlatformIO`.
2. Verify real endpoints and behavior with a real `Bose SA-5`.
3. Remap pins according to the final hardware.
4. Verify encoder direction and button debounce.
5. Test setup flow, reconnect behavior, and `Wi-Fi` outage handling.
6. Optionally add a visual wiring diagram and an exact front-panel design.
