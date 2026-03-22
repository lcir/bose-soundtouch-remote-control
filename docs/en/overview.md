# Project Overview

## Goal

The goal of this project is to build a standalone physical controller for the `Bose SoundTouch SA-5` that works without a phone and without depending on Bose cloud services. The controller is based on an `ESP32` and controls the amplifier over the local `Wi-Fi` network.

## Device Features

- home `Wi-Fi` and `Bose` host setup through a captive portal
- basic status display on a `128x64 OLED`
- volume control with a rotary encoder
- source switching with a single button
- sending a `standby/off` command
- status indication with a two-color `red/green` LED
- reading current volume, source, and `now playing`
- reacting to Bose state changes over `WebSocket`
- local web control from a phone or laptop on the same `Wi-Fi`

## Architecture

The system is split into five main parts:

- `InputController`
  - reads the encoder and button states
  - handles button debounce
- `BoseClient`
  - sends `HTTP` commands to Bose
  - reads `/sources`, `/volume`, `/now_playing`
  - maintains the `WebSocket` connection and triggers state refreshes
- `CaptivePortal`
  - starts an `AP` on first boot
  - stores `SSID`, password, and `Bose host/IP` through the web UI
- `ControlWebServer`
  - provides a LAN web UI in normal mode
  - shows status and calls the same actions as the physical controls
- `UiRenderer`
  - draws the normal operation screen
  - draws the setup mode screen

## Operating Modes

### 1. Setup Mode

Used on first boot or after holding the service button during boot. The `ESP32` creates its own `AP` and waits for configuration to be saved.

### 2. Normal Mode

The `ESP32` joins the home network, connects to the `Bose SA-5`, keeps the display updated, and at the same time exposes a simple web control interface on the local network.

## Event Flow

1. The user turns the encoder, presses a button, or uses the local web UI.
2. The `ESP32` converts the action into a Bose network command.
3. Bose returns state via `HTTP` or pushes a change over `WebSocket`.
4. The `ESP32` fetches the latest data and redraws the display.

## `v1` Limitations

- only one target `Bose` device
- no automatic discovery on the network
- no cloud presets or music service browsing
- no physical power disconnection of the amplifier
