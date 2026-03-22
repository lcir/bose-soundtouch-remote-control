# Bose SoundTouch SA-5 ESP32 Remote

ESP32 firmware for a dedicated Bose SoundTouch SA-5 wall/desk controller.

Detailed project documentation is in [docs/README.md](/Users/peny/Development/Projects/boser-remote-control/docs/README.md).

Mechanical enclosure CAD is in [cad/bose_remote_enclosure.scad](/Users/peny/Development/Projects/boser-remote-control/cad/bose_remote_enclosure.scad).

The project now defaults to `LOLIN/Wemos S2 Mini` for prototyping.

## Features

- Captive portal setup for Wi-Fi and Bose hostname/IP
- Bose SoundTouch local control over HTTP (`8090`) and WebSocket (`8080`)
- Built-in LAN web UI for phone/browser control
- Rotary encoder volume control with request coalescing
- Single source-cycle button based on live `/sources`
- Dedicated standby button for turning SoundTouch off
- Bi-color power LED: `red = standby/off`, `green = on`
- `128x64` OLED status UI (`SSD1306` over `I2C`)

## Default wiring

Current defaults are for `LOLIN/Wemos S2 Mini` and a generic `0.96" 128x64 I2C OLED`.
Change these in `include/PinConfig.h` if your hardware differs.

- `GPIO33` `SDA`
- `GPIO35` `SCL`
- `GPIO7` encoder `A`
- `GPIO9` encoder `B`
- `GPIO5` source cycle button
- `GPIO11` standby button / setup hold button
- `GPIO16` power LED `red`
- `GPIO18` power LED `green`

Buttons are assumed `active low` with `INPUT_PULLUP`.

## Setup flow

1. Flash firmware.
2. On first boot, connect to Wi-Fi `BoseRemote-xxxx`.
3. Open `http://192.168.4.1`.
4. Enter home Wi-Fi credentials and Bose `hostname` or `IP`.
5. Device reboots into normal station mode.

Holding the standby button during boot for roughly `1.5 s` forces setup mode again.

## LAN Web UI

In normal station mode, the ESP32 also serves a local control page on its own IP address.
Open `http://<esp32-ip>/` in a browser on the same network to:

- change volume
- switch to the next logical source, including `AUX`, `Bluetooth` and grouped `Online`
- directly select the grouped sources exposed by the web UI
- send dynamic `Wake` / `Standby`
- view current Bose state

## Build

This repo is prepared for `PlatformIO`.

```bash
pio run
pio run -t upload
pio device monitor
```

The default environment is `lolin_s2_mini`. A compatibility target for generic classic ESP32 boards is still available via:

```bash
pio run -e esp32dev
```
