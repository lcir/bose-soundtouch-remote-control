# Firmware a provoz

## Vývojové prostředí

Projekt je připravený pro `PlatformIO` s `Arduino` frameworkem.

- konfigurace build prostředí: [platformio.ini](/Users/peny/Development/Projects/boser-remote-control/platformio.ini)
- hlavní vstup firmware: [main.cpp](/Users/peny/Development/Projects/boser-remote-control/src/main.cpp)
- výchozí build target: `lolin_s2_mini`

## Použité knihovny

- `U8g2` pro `SSD1306 OLED`
- `WebSockets` pro Bose notifikace
- `ESP32Encoder` pro rotační enkodér
- `Preferences` pro perzistentní konfiguraci
- `WiFi`, `WebServer`, `DNSServer`, `HTTPClient`

## Build a flash

```bash
cd /Users/peny/Development/Projects/boser-remote-control
pio run
pio run -t upload
pio device monitor
```

Pokud `pio` není v `PATH`, lze použít:

```bash
~/.local/bin/pio run
~/.local/bin/pio run -t upload
~/.local/bin/pio device monitor
```

Pro kompatibilní klasický `ESP32` target je v projektu stále i alternativní environment:

```bash
pio run -e esp32dev
```

Poznámka pro `LOLIN/Wemos S2 Mini`:

- build zapíná `USB CDC on boot`
- `Serial` tedy běží přes nativní `USB`, ne přes externí `USB-UART`

## Konfigurace

Konfigurace se ukládá do `NVS` přes `Preferences`.

Ukládané položky:

- `wifi_ssid`
- `wifi_pwd`
- `bose_host`
- `http_port`
- `ws_port`
- `display_flip`

Datový model je definovaný v [Types.h](/Users/peny/Development/Projects/boser-remote-control/include/Types.h) a ukládání v [ConfigStore.cpp](/Users/peny/Development/Projects/boser-remote-control/src/ConfigStore.cpp).

## Setup flow

1. Po prvním bootu se spustí `AP` `BoseRemote-xxxx`.
2. Uživatel se připojí telefonem nebo notebookem.
3. Otevře `http://192.168.4.1`.
4. Zadá domácí `Wi‑Fi` a `Bose host/IP`.
5. `ESP32` uloží konfiguraci a přepne se do normálního režimu.

Servisní návrat do setup režimu:

- při startu podržet tlačítko encoderu přibližně `1.5 s`

## Web rozhraní

Firmware používá dva web režimy:

- `CaptivePortal`
  - aktivní pouze v setup režimu
  - běží na `192.168.4.1`
  - slouží jen pro uložení `Wi‑Fi` a `Bose host/IP`
- `ControlWebServer`
  - aktivní v normálním režimu
  - běží na IP adrese přidělené v domácí síti
  - poskytuje jednoduché mobilní ovládání a stavový přehled

Endpointy lokálního ovládacího webu:

- `GET /`
- `GET /api/state`
- `POST /api/power`
- `POST /api/source/next`
- `POST /api/source/select`
- `POST /api/standby`
- `POST /api/volume`

## Bose komunikace

### HTTP

Použité endpointy:

- `/info`
- `/sources`
- `/volume`
- `/now_playing`
- `/select`
- `/key`

### WebSocket

- host: stejný jako `Bose host`
- port: `8080`
- path: `/`
- subprotocol: `gabbo`

Při přijetí notifikací `volumeUpdated`, `sourcesUpdated`, `nowPlayingUpdated` firmware nenačítá stav z payloadu přímo, ale provede nový `HTTP` refresh.

## Logika ovládání

### Enkodér

- každý krok mění cílovou hlasitost o `1`
- odeslání na Bose je zpožděné o `100 ms`
- rychlé otáčení tedy nevytváří desítky bezprostředních requestů
- krátký stisk otevře menu na OLED
- menu má položky `Volume`, `Source`, `Power`
- po potvrzení submenu se menu zavře a ovládání se vrátí na hlasitost
- při neaktivitě se menu po několika sekundách samo zavře

### Tlačítko encoderu

- používá se pro otevření menu a potvrzení volby
- při bootu slouží i jako servisní hold pro návrat do setup režimu

### Web UI

- web volá stejnou Bose logiku jako fyzická tlačítka
- stránka pravidelně načítá stav přes `/api/state`
- hlasitost se odesílá přes krátce zpožděné `POST /api/volume`
- hlavní power tlačítko se přepíná mezi `Wake` a `Standby`
- barva power tlačítka sleduje stav:
  - `red` pro probuzení ze standby
  - `green` pro uspání
- wake se snaží zařízení probudit přes `POWER` key a v případě potřeby použije fallback přes výběr vhodného zdroje
- rotace zdrojů i webový seznam zahrnují vedle `AUX` i `Bluetooth` a jednu seskupenou položku `Online`
- syrové položky jako `QPlay` nebo technické placeholdery se ve webovém UI nezobrazují

### Stavová LED

- LED svítí, když je Bose zapnutý a dostupný
- LED je zhasnutá, když je Bose ve `standby/off` stavu nebo není dostupný

### Displej

Normální obrazovka ukazuje:

- stav `Wi‑Fi`
- stav `WebSocket`
- aktivní zdroj
- hlasitost a `mute`
- krátký řádek `artist/track`

Menu obrazovka ukazuje:

- název menu nebo submenu
- zvýrazněnou aktuální položku
- stručnou nápovědu nebo stavovou hlášku na spodním řádku

## Chování při chybách

- při výpadku `Wi‑Fi` se zařízení pokouší znovu připojit
- při výpadku `WebSocket` přechází do poll režimu po `5 s`
- při obnovení připojení se znovu otevírá Bose session a načítá aktuální stav

## Co bude pravděpodobně potřeba doladit na reálném hardware

- směr enkodéru
- konkrétní piny podle použité desky
- orientace OLED
- mechanické umístění encoderu a jeho tlačítka
- latence a případný krok hlasitosti
