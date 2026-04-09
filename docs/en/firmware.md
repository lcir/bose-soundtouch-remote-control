# Firmware and Operation

## Development Environment

The project is prepared for `PlatformIO` with the `Arduino` framework.

- build configuration: [platformio.ini](/Users/peny/Development/Projects/boser-remote-control/platformio.ini)
- main firmware entry point: [main.cpp](/Users/peny/Development/Projects/boser-remote-control/src/main.cpp)
- default build target: `lolin_s2_mini`

## Libraries Used

- `U8g2` for the `SSD1306 OLED`
- `WebSockets` for Bose notifications
- `ESP32Encoder` for the rotary encoder
- `Preferences` for persistent configuration
- `WiFi`, `WebServer`, `DNSServer`, `HTTPClient`

## Build and Flash

```bash
cd /Users/peny/Development/Projects/boser-remote-control
pio run
pio run -t upload
pio device monitor
```

If `pio` is not in `PATH`, you can use:

```bash
~/.local/bin/pio run
~/.local/bin/pio run -t upload
~/.local/bin/pio device monitor
```

An alternative environment for a compatible classic `ESP32` target is still present in the project:

```bash
pio run -e esp32dev
```

Note for `LOLIN/Wemos S2 Mini`:

- the build enables `USB CDC on boot`
- `Serial` therefore runs over native `USB`, not an external `USB-UART`

## Configuration

Configuration is stored in `NVS` using `Preferences`.

Stored items:

- `wifi_ssid`
- `wifi_pwd`
- `bose_host`
- `http_port`
- `ws_port`
- `display_flip`

The data model is defined in [Types.h](/Users/peny/Development/Projects/boser-remote-control/include/Types.h) and persistence is implemented in [ConfigStore.cpp](/Users/peny/Development/Projects/boser-remote-control/src/ConfigStore.cpp).

## Setup Flow

1. On first boot the `BoseRemote-xxxx` `AP` starts.
2. The user connects using a phone or laptop.
3. They open `http://192.168.4.1`.
4. They enter home `Wi-Fi` and `Bose host/IP`.
5. The `ESP32` stores the configuration and switches to normal mode.

Service return to setup mode:

- hold the encoder button during startup for roughly `1.5 s`

## Web Interface

The firmware uses two web modes:

- `CaptivePortal`
  - active only in setup mode
  - runs on `192.168.4.1`
  - used only to store `Wi-Fi` and `Bose host/IP`
- `ControlWebServer`
  - active in normal mode
  - runs on the IP address assigned on the home network
  - provides simple mobile control and a status overview

Local control web endpoints:

- `GET /`
- `GET /api/state`
- `POST /api/power`
- `POST /api/source/next`
- `POST /api/source/select`
- `POST /api/standby`
- `POST /api/volume`

## Bose Communication

### HTTP

Used endpoints:

- `/info`
- `/sources`
- `/volume`
- `/now_playing`
- `/select`
- `/key`

### WebSocket

- host: same as `Bose host`
- port: `8080`
- path: `/`
- subprotocol: `gabbo`

When notifications such as `volumeUpdated`, `sourcesUpdated`, or `nowPlayingUpdated` are received, the firmware does not read the state directly from the payload. It performs a fresh `HTTP` refresh instead.

## Control Logic

### Encoder

- every step changes the target volume by `1`
- sending to Bose is delayed by `100 ms`
- fast rotation therefore does not create dozens of immediate requests
- a short press opens the OLED menu
- the menu contains `Volume`, `Source`, and `Power`
- confirming a submenu choice closes the menu and returns control to idle volume mode
- the menu closes automatically after a few seconds of inactivity

### Encoder Button

- opens the menu and confirms selections
- also serves as the boot-time service hold for returning to setup mode

### Web UI

- the web UI calls the same Bose logic as the physical controls
- the page periodically fetches state through `/api/state`
- volume is sent through a slightly delayed `POST /api/volume`
- the main power button toggles between `Wake` and `Standby`
- the power button color follows the state:
  - `green` for waking from standby
  - `red` for putting the device into standby
- source rotation and the web list include `AUX`, `Bluetooth`, and suitable online sources if Bose exposes them through `/sources`

### Status LED

- the LED is on when Bose is powered on and available
- the LED is off when Bose is in `standby/off` state or unavailable

### Display

The normal screen shows:

- `Wi-Fi` status
- `WebSocket` status
- current source
- volume and `mute`
- a short `artist/track` line

The menu screen shows:

- the menu or submenu title
- the currently highlighted item
- a short hint or status line at the bottom

## Error Handling

- when `Wi-Fi` drops, the device tries to reconnect
- when `WebSocket` drops, it switches to polling mode after `5 s`
- when connectivity returns, it reopens the Bose session and reloads current state

## Things Likely To Need Tuning On Real Hardware

- encoder direction
- exact pins for the board in use
- OLED orientation
- physical placement of the encoder and its push button
- latency and possible volume step tuning
