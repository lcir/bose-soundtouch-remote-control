# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32 firmware for a physical remote controller for the **Bose SoundTouch SA-5** amplifier. All Bose communication is local-network only (no cloud) using the SoundTouch HTTP API (port 8090) and WebSocket push notifications (port 8080, subprotocol `gabbo`).

## Build Commands

This project uses **PlatformIO**. There are no unit tests.

```bash
pio run                          # Build default target (lolin_s2_mini)
pio run -e esp32dev              # Build for generic ESP32
pio run -t upload                # Build and upload to device
pio run -e lolin_s2_mini -t upload
pio run -e esp32dev -t upload
pio device monitor               # Serial monitor at 115200 baud
```

## Architecture

The firmware is a single `BoseRemoteApp` class (`src/main.cpp`) that orchestrates 5 modules:

- **`BoseClient`** — All Bose HTTP/WebSocket communication. XML parsing is hand-rolled (no library). WebSocket events schedule HTTP refreshes rather than parsing WS payload directly. Falls back to 5-second polling when WebSocket is down.
- **`CaptivePortal`** — First-boot AP mode (`BoseRemote-xxxx`), DNS redirect, and web form for Wi-Fi + Bose host configuration.
- **`ControlWebServer`** — LAN web UI on port 80 with REST API (`/api/state`, `/api/power`, `/api/source/next`, `/api/source/select`, `/api/standby`, `/api/volume`). The HTML/CSS/JS page is built as inline strings.
- **`InputController`** — Rotary encoder (half-quad, internal pull-ups), 25ms button debounce, power-held-at-boot detection for forced setup mode.
- **`UiRenderer`** — SSD1306 128x64 OLED over I2C via U8g2.
- **`ConfigStore`** — Persists `DeviceConfig` to ESP32 NVS via Arduino `Preferences` (namespace `bose-remote`).

Shared data structures (`DeviceConfig`, `BoseSource`, `BoseState`) are in `include/Types.h`.

## Key Behaviors

- **Volume coalescing**: encoder deltas accumulate and the HTTP call fires after 100ms idle.
- **OLED rate-limited** to 80ms render interval.
- **Source grouping**: multiple AUX inputs → individual entries; all non-local/non-BT online sources → single `ONLINE` selection; QPlay sources are hidden.
- **Two runtime modes**: setup mode (captive portal) vs. normal mode (Bose control).

## Hardware Configuration

All GPIO pin assignments and hardware constants are in `include/PinConfig.h`. Change this file to adapt to a different board. The default target is the LOLIN/Wemos S2 Mini (`lolin_s2_mini` env in `platformio.ini`).

Runtime config (Wi-Fi credentials, Bose host) is stored on-device in NVS — not in source files.
