# Bill of Materials

Tento kusovník je připravený pro `v1` ovladač `Bose SoundTouch SA-5` s `ESP32`, `OLED`, rotačním enkodérem, dvěma tlačítky a dvoubarevnou stavovou LED.

## Varianta A: rychlý prototyp na stole

| Položka | Qty | Doporučený typ | Poznámka |
|---|---:|---|---|
| `ESP32` vývojová deska | 1 | `LOLIN/Wemos S2 Mini` | Výchozí deska pro první prototyp |
| OLED displej | 1 | `SSD1306 0.96" 128x64 I2C` | 4 vodiče, jednoduché zapojení |
| Rotační enkodér | 1 | LaskaKit `Rotační encoder s tlačítkem a RC s filtrem`, `LA132020` | Výchozí volba pro první prototyp |
| Knoflík na enkodér | 1 | pro hřídel dle zvoleného enkodéru | Typicky `6 mm` |
| Tlačítko `Source` | 1 | momentary `NO` pushbutton | Aktivní v `LOW` proti `GND` |
| Tlačítko `Standby` | 1 | momentary `NO` pushbutton | Aktivní v `LOW` proti `GND` |
| Stavová LED | 1 | dvoubarevná `red/green` LED nebo podsvícené tlačítko s dvěma LED větvemi | Výchozí firmware čeká `red` a `green` kanál zvlášť |
| Rezistory pro LED | 2 | `220R` až `1k` | Jeden pro červenou a jeden pro zelenou větev |
| Breadboard | 1 | half-size nebo podobný | Na první oživení |
| Jumper vodiče | 1 sada | `Dupont` / breadboard wires | Male-female nebo male-male podle desky |
| USB kabel | 1 | podle desky, pro `LOLIN/Wemos S2 Mini` typicky `USB-C` | Napájení a flash |
| `5 V USB` napájení | 1 | běžný USB adaptér | Pro prototyp stačí telefonní nabíječka |

## Varianta B: čistší vestavba do krabičky

| Položka | Qty | Doporučený typ | Poznámka |
|---|---:|---|---|
| `ESP32` vývojová deska | 1 | stejná jako u prototypu | Může zůstat i ve finální verzi |
| OLED displej | 1 | `SSD1306 0.96" 128x64 I2C` | V panelu vyříznout okno |
| Rotační enkodér | 1 | `Bourns PEC11R` | Lepší mechanická jistota |
| Knoflík na enkodér | 1 | panelový knob | Volit dle estetiky a hřídele |
| Panelová tlačítka | 2 | momentary `NO`, ideálně panel mount `16 mm` | `Source`, `Standby` |
| Stavová LED nebo iluminované tlačítko | 1 | `red/green` | Pro indikaci `standby/on` |
| Rezistory pro LED | 2 | `220R` až `1k` | Sériově do LED větví |
| Prototypovací PCB | 1 | `Perma-Proto` nebo univerzální děrovaný spoj | Na trvalejší propojení |
| Krabička | 1 | plastová `ABS` krabička | Pro panel a mechanickou ochranu |
| Sloupky / distančníky | 4 | `M2.5` nebo `M3` | Dle otvorů displeje a desky |
| Šroubky a matice | sada | `M2.5/M3` | Montáž do krabičky |
| Vodič 24 až 26 AWG | 1 sada | lankový nebo pevný | Pro vnitřní propoje |
| Konektory / pin headery | dle potřeby | `2.54 mm` | Usnadní servis a výměny |
| `5 V USB` zdroj | 1 | trvale připojený adaptér | Napájení finální jednotky |

## Doporučené konkrétní díly

### `ESP32`

- preferovaný typ pro aktuální prototyp: `LOLIN/Wemos S2 Mini`
- alternativa: jiná kvalitní `ESP32` nebo `ESP32-S2` dev deska s `3.3 V` logikou a vyvedenými GPIO

Poznámka:

- aktuální firmware je přizpůsobený na `LOLIN/Wemos S2 Mini`
- výchozí build target v `PlatformIO` je `lolin_s2_mini`

### OLED

- doporučený modul: `SSD1306 128x64`, `0.96"`, `I2C`
- preferovat modul, který spolehlivě funguje na `3.3 V`

### Enkodér

- pro prototyp: LaskaKit `Rotační encoder s tlačítkem a RC s filtrem`, `LA132020`
- parametry prototypové varianty: `20 pulzů na otáčku`, integrované tlačítko, RC filtr proti rušení
- pro finálnější mechaniku: `Bourns PEC11R`
- důvod: RC filtrovaný modul je vhodný pro rychlé oživení na `ESP32`, `Bourns` je mechanicky lepší pro hotové zařízení

### Tlačítka

- prototyp: obyčejná momentary `NO` tlačítka
- finální panel: panel mount tlačítka, ideálně `16 mm`

### Stavová LED

- doporučená varianta: dvoubarevná `red/green` LED
- alternativa: podsvícené `standby` tlačítko se samostatným `red` a `green` vstupem

## Orientační rozdělení na povinné a volitelné položky

### Povinné pro funkční prototyp

- `ESP32`
- `OLED`
- enkodér
- 2 tlačítka
- stavová LED
- 2 LED rezistory
- vodiče
- napájení

### Volitelné, ale doporučené

- prototypovací PCB
- panel mount tlačítka
- knob na enkodér
- krabička
- distanční sloupky

## Piny a návaznost na BOM

Výchozí pinout firmware:

- `GPIO33` -> `OLED SDA`
- `GPIO35` -> `OLED SCL`
- `GPIO7` -> `Encoder A`
- `GPIO9` -> `Encoder B`
- `GPIO5` -> `Source`
- `GPIO11` -> `Standby`
- `GPIO16` -> `LED Red`
- `GPIO18` -> `LED Green`

Detail zapojení je v [wiring.md](/Users/peny/Development/Projects/boser-remote-control/docs/wiring.md).

## Doporučení k nákupu

- pro první prototyp nekupovat custom PCB
- začít na existující `LOLIN/Wemos S2 Mini`, kterou už máš
- pokud si nejsi jistý mechanikou, vzít levnější generické tlačítka pro prototyp a kvalitnější panelové díly až do finální krabičky
- pokud bude zařízení v kovové skříni, počítat s horším `Wi‑Fi` a případně upravit polohu `ESP32`

## Referenční díly a orientační ceny

Níže jsou příklady aktuálně dohledatelných dílů, které odpovídají tomuto návrhu. Ceny jsou jen orientační a mohou se lišit podle distributora, DPH a dopravy.

| Položka | Příklad | Orientačně |
|---|---|---:|
| OLED `128x64 I2C` | `Adafruit 0.96" SSD1306`, Product ID `326` | `USD 17.50` |
| Prototypový enkodér | `LaskaKit Rotační encoder s tlačítkem a RC s filtrem`, `LA132020` | `48 Kč` |
| Panelové tlačítko `16 mm` | `Adafruit 16mm Panel Mount Momentary Pushbutton`, Product ID `1505/1445/...` | `USD 0.95 / ks` |
| Jumper wires | `Adafruit Premium Male/Male 40x6"`, Product ID `758/3142` | `USD 3.95` |
| Prototypovací PCB | `Adafruit Perma-Proto Quarter-sized 3-pack`, Product ID `589` | `USD 8.50 / 3 ks` |
| Krabička | `Hammond 1591` série | cena dle varianty a distributora |
| Robustnější enkodér | `Bourns PEC11R` | cena dle konkrétní konfigurace a distributora |
| `ESP32` dev board | `LOLIN/Wemos S2 Mini` | cena dle distributora |
