# Přehled projektu

## Cíl

Cílem projektu je postavit samostatný fyzický ovladač pro `Bose SoundTouch SA-5`, který bude fungovat bez telefonu a bez závislosti na cloudových službách Bose. Ovladač je založený na `ESP32` a ovládá zesilovač přes lokální `Wi‑Fi` síť.

## Co zařízení umí

- nastavení domácí `Wi‑Fi` a `Bose` hostu přes captive portal
- zobrazení základního stavu na `OLED 128x64`
- změnu hlasitosti rotačním enkodérem
- ovládání `Volume`, `Source` a `Power` přes menu na tlačítku encoderu
- indikaci stavu přes jednu stavovou LED
- čtení aktuální hlasitosti, zdroje a `now playing`
- reakci na změny stavu z Bose přes `WebSocket`
- lokální webové ovládání z telefonu nebo notebooku v téže `Wi‑Fi`
- webové tlačítko `Wake/Standby` podle aktuálního stavu Bose
- webový seznam zdrojů seskupený do přehledných voleb `AUX`, `Bluetooth` a `Online`

## Architektura

Systém je rozdělený na pět hlavních částí:

- `InputController`
  - čte stav enkodéru a tlačítka encoderu
  - dělá debounce stisku
- `BoseClient`
  - odesílá `HTTP` příkazy na Bose
  - čte `/sources`, `/volume`, `/now_playing`
  - drží `WebSocket` spojení a vyvolává refresh stavů
- `CaptivePortal`
  - při prvním startu vytvoří `AP`
  - přes web uloží `SSID`, heslo a `Bose host/IP`
- `ControlWebServer`
  - v normálním režimu poskytuje LAN web UI
  - zobrazuje stav a volá stejné akce jako fyzické ovládání
  - seskupuje syrové Bose zdroje do jednoduššího webového modelu
- `UiRenderer`
  - vykresluje normální provozní obrazovku
  - vykresluje setup režim

## Provozní režimy

### 1. Setup režim

Použije se při prvním startu nebo po servisním podržení tlačítka encoderu při bootu. `ESP32` vytvoří vlastní `AP` a čeká na uložení konfigurace.

### 2. Normální režim

`ESP32` se připojí do domácí sítě, naváže spojení s `Bose SA-5`, průběžně aktualizuje stav na displeji a současně vystaví jednoduché webové ovládání v lokální síti.

## Tok událostí

1. Uživatel otočí enkodérem, klikne encoder nebo použije lokální web.
2. `ESP32` převede akci na síťový příkaz Bose.
3. Bose vrátí stav přes `HTTP` nebo pošle změnu přes `WebSocket`.
4. `ESP32` načte aktuální data a překreslí displej.

## Omezení `v1`

- pouze jedno cílové `Bose` zařízení
- bez automatického vyhledávání v síti
- bez cloudových presetů a procházení hudebních služeb
- bez fyzického odpojování napájení zesilovače
