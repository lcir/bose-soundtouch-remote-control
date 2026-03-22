# Plan: Bose SoundTouch SA-5 ESP32 Remote

## Cíl

Postavit samostatný fyzický ovladač pro `Bose SoundTouch SA-5`, který bude přes `ESP32` ovládat zesilovač po `Wi‑Fi` bez nutnosti používat telefon nebo Bose aplikaci.

## Zvolený směr

- firmware bude v `C++`
- build a správa projektu budou přes `PlatformIO`
- framework bude `Arduino` pro `ESP32`
- výchozí prototypovací deska bude `LOLIN/Wemos S2 Mini`
- zařízení bude ovládat jedno konkrétní `Bose SoundTouch SA-5`
- Bose bude připojené pouze přes lokální `Wi‑Fi`

## Hardware v1

- `LOLIN/Wemos S2 Mini`
- `OLED SSD1306 128x64` přes `I2C`
- LaskaKit rotační enkodér s tlačítkem a `RC` filtrem (`LA132020`) pro první prototyp
- tlačítko `Source`
- tlačítko `Standby`
- dvoubarevná LED `red/green` pro indikaci stavu

## Ovládání

- enkodér mění hlasitost
- změny hlasitosti se coalescují a odesílají po krátké prodlevě
- `Source` přepíná na další dostupný zdroj
- `Standby` vypíná `SoundTouch`
- stav zařízení se zobrazuje na dvoubarevné LED
- stejné základní ovládání bude dostupné i přes lokální web

## UI

Na `OLED` displeji se má zobrazovat:

- stav `Wi‑Fi`
- stav `WebSocket` spojení
- aktuální zdroj
- hlasitost
- stav `mute`
- krátký text `artist/track`, pokud je dostupný

Při změně zdroje nebo hlasitosti se krátce zobrazí zvýrazněný overlay.

LED logika:

- `červená`: `standby/off`
- `zelená`: `on`

## Setup flow

- při prvním startu nebo chybějící konfiguraci přejde zařízení do setup režimu
- `ESP32` vytvoří vlastní `AP`
- uživatel se připojí na captive portal
- uloží se `Wi‑Fi SSID`, heslo a `Bose host/IP`
- konfigurace se uloží do `NVS`
- podržení `Standby` tlačítka při bootu vrátí zařízení do setup režimu
- v normálním režimu bude `ESP32` vystavovat jednoduchý ovládací web v LAN

## Bose komunikace

Použít lokální `SoundTouch` API:

- `HTTP` na portu `8090`
- `WebSocket` na portu `8080`
- subprotocol `gabbo`

Použité endpointy:

- `/info`
- `/sources`
- `/volume`
- `/now_playing`
- `/select`
- `/key`

Pravidla:

- zdroje načítat dynamicky z `/sources`
- přepínat jen zdroje ve stavu `READY`
- změny z websocketu neparsovat přímo do stavu, ale vyvolat refresh přes `HTTP`
- při pádu websocketu přejít do poll režimu
- pro vypnutí použít explicitní `standby/off`, ne `POWER` toggle
- lokální web bude používat stejné interní akce jako fyzické ovládání

## Elektrické zapojení v1

- `GPIO33` -> OLED `SDA`
- `GPIO35` -> OLED `SCL`
- `GPIO7` -> encoder `A`
- `GPIO9` -> encoder `B`
- `GPIO5` -> tlačítko `Source`
- `GPIO11` -> tlačítko `Standby`
- `GPIO16` -> LED `Red`
- `GPIO18` -> LED `Green`

Předpoklady:

- tlačítka jsou zapojená proti `GND`
- firmware používá `INPUT_PULLUP`
- `OLED` je napájený z `3.3 V`
- Bose není propojený vodičem, pouze přes `Wi‑Fi`

## Omezení v1

- bez automatického discovery Bose zařízení v síti
- bez cloudových presetů a služeb Bose
- bez fyzického vypínání napájení zesilovače
- bez zásahu do elektroniky `SA-5`

## Aktuální stav implementace

V repozitáři je připravená první implementace:

- `PlatformIO` projekt
- captive portal
- lokální ovládací web
- Bose `HTTP` a `WebSocket` klient
- OLED renderer
- input vrstva pro enkodér a tlačítka
- základní dokumentace v `docs/`

## Další praktické kroky

1. Ověřit build na `LOLIN/Wemos S2 Mini` přes `PlatformIO`.
2. Ověřit skutečné endpointy a chování s reálným `Bose SA-5`.
3. Přemapovat piny podle finálního hardware.
4. Ověřit směr enkodéru a debounce tlačítek.
5. Otestovat setup flow, reconnect a chování při výpadku `Wi‑Fi`.
6. Případně doplnit obrázkové schéma zapojení a přesný návrh čelního panelu.
