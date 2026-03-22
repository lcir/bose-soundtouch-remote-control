# Project Documentation

This directory contains the English technical documentation for the `ESP32` controller for the `Bose SoundTouch SA-5` amplifier.

## Contents

- [Project Overview](/Users/peny/Development/Projects/boser-remote-control/docs/en/overview.md)
- [Firmware and Operation](/Users/peny/Development/Projects/boser-remote-control/docs/en/firmware.md)
- [Wiring and Connections](/Users/peny/Development/Projects/boser-remote-control/docs/en/wiring.md)
- [Bill of Materials](/Users/peny/Development/Projects/boser-remote-control/docs/en/bom.md)

## Quick Orientation

- The `ESP32` communicates with the `Bose SA-5` only over `Wi-Fi`.
- There is no control wire, relay, or analog potentiometer connected to Bose.
- All physical controls are connected only to the `ESP32`.
- The `ESP32` sends local network commands to Bose over `HTTP` and listens for changes over `WebSocket`.
- In normal mode the `ESP32` also provides a local web control UI on the LAN.

## Project Files

- Main firmware: [main.cpp](/Users/peny/Development/Projects/boser-remote-control/src/main.cpp)
- Bose communication: [BoseClient.cpp](/Users/peny/Development/Projects/boser-remote-control/src/BoseClient.cpp)
- Setup portal: [CaptivePortal.cpp](/Users/peny/Development/Projects/boser-remote-control/src/CaptivePortal.cpp)
- LAN web UI: [ControlWebServer.cpp](/Users/peny/Development/Projects/boser-remote-control/src/ControlWebServer.cpp)
- Inputs: [InputController.cpp](/Users/peny/Development/Projects/boser-remote-control/src/InputController.cpp)
- OLED UI: [UiRenderer.cpp](/Users/peny/Development/Projects/boser-remote-control/src/UiRenderer.cpp)
- Pins: [PinConfig.h](/Users/peny/Development/Projects/boser-remote-control/include/PinConfig.h)

## Notes

- The original Czech documentation remains available in the parent `docs/` directory.
- The enclosure document linked from the Czech index is not currently present in the repository, so it is not mirrored here.
