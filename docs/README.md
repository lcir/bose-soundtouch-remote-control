# Dokumentace projektu

Tento adresář obsahuje technickou dokumentaci pro `ESP32` ovladač zesilovače `Bose SoundTouch SA-5`.

English version: [docs/en/README.md](/Users/peny/Development/Projects/boser-remote-control/docs/en/README.md)

## Obsah

- [Přehled projektu](/Users/peny/Development/Projects/boser-remote-control/docs/overview.md)
- [Firmware a provoz](/Users/peny/Development/Projects/boser-remote-control/docs/firmware.md)
- [Zapojení a spojení vodičů](/Users/peny/Development/Projects/boser-remote-control/docs/wiring.md)
- [Bill of Materials](/Users/peny/Development/Projects/boser-remote-control/docs/bom.md)
- [Návrh krabičky](/Users/peny/Development/Projects/boser-remote-control/docs/enclosure.md)

## Rychlá orientace

- `ESP32` komunikuje s `Bose SA-5` pouze přes `Wi‑Fi`.
- Na Bose se nepřipojuje žádný ovládací vodič, relé ani analogový potenciometr.
- Všechny fyzické ovládací prvky jsou připojené jen k `ESP32`.
- `ESP32` posílá do Bose lokální síťové příkazy přes `HTTP` a poslouchá změny přes `WebSocket`.
- V normálním režimu `ESP32` zároveň poskytuje lokální webové ovládání v LAN.
- Web aktuálně nabízí dynamické `Wake/Standby` a seskupené zdroje `AUX`, `Bluetooth`, `Online`.
- [wiring.md](/Users/peny/Development/Projects/boser-remote-control/docs/wiring.md) nově obsahuje i obrazek s projektovym pinoutem `LOLIN/Wemos S2 Mini` a napojenymi komponentami.

## Soubory v projektu

- Hlavní firmware: [main.cpp](/Users/peny/Development/Projects/boser-remote-control/src/main.cpp)
- Bose komunikace: [BoseClient.cpp](/Users/peny/Development/Projects/boser-remote-control/src/BoseClient.cpp)
- Setup portal: [CaptivePortal.cpp](/Users/peny/Development/Projects/boser-remote-control/src/CaptivePortal.cpp)
- LAN web UI: [ControlWebServer.cpp](/Users/peny/Development/Projects/boser-remote-control/src/ControlWebServer.cpp)
- Vstupy: [InputController.cpp](/Users/peny/Development/Projects/boser-remote-control/src/InputController.cpp)
- OLED UI: [UiRenderer.cpp](/Users/peny/Development/Projects/boser-remote-control/src/UiRenderer.cpp)
- Piny: [PinConfig.h](/Users/peny/Development/Projects/boser-remote-control/include/PinConfig.h)
