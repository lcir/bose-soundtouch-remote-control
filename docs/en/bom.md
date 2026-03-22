# Bill of Materials

This bill of materials is prepared for the `v1` controller for the `Bose SoundTouch SA-5` using an `ESP32`, `OLED`, rotary encoder, two buttons, and a two-color status LED.

## Variant A: Quick Prototype On The Desk

| Item | Qty | Recommended Type | Note |
|---|---:|---|---|
| `ESP32` development board | 1 | `LOLIN/Wemos S2 Mini` | Default board for the first prototype |
| OLED display | 1 | `SSD1306 0.96" 128x64 I2C` | 4 wires, simple wiring |
| Rotary encoder | 1 | LaskaKit `Rotacni encoder s tlacitkem a RC s filtrem`, `LA132020` | Default choice for the first prototype |
| Encoder knob | 1 | according to the selected encoder shaft | Typically `6 mm` |
| `Source` button | 1 | momentary `NO` pushbutton | Active in `LOW` against `GND` |
| `Standby` button | 1 | momentary `NO` pushbutton | Active in `LOW` against `GND` |
| Status LED | 1 | two-color `red/green` LED or illuminated button with two LED branches | Default firmware expects separate `red` and `green` channels |
| LED resistors | 2 | `220R` to `1k` | One for the red and one for the green branch |
| Breadboard | 1 | half-size or similar | For initial bring-up |
| Jumper wires | 1 set | `Dupont` / breadboard wires | Male-female or male-male depending on the board |
| USB cable | 1 | according to the board, typically `USB-C` for `LOLIN/Wemos S2 Mini` | Power and flashing |
| `5 V USB` power supply | 1 | standard USB adapter | A phone charger is enough for a prototype |

## Variant B: Cleaner Enclosure Build

| Item | Qty | Recommended Type | Note |
|---|---:|---|---|
| `ESP32` development board | 1 | same as the prototype | Can remain in the final unit |
| OLED display | 1 | `SSD1306 0.96" 128x64 I2C` | Cut a panel window for it |
| Rotary encoder | 1 | `Bourns PEC11R` | Better mechanical feel |
| Encoder knob | 1 | panel knob | Choose to match style and shaft |
| Panel buttons | 2 | momentary `NO`, ideally panel mount `16 mm` | `Source`, `Standby` |
| Status LED or illuminated button | 1 | `red/green` | For `standby/on` indication |
| LED resistors | 2 | `220R` to `1k` | In series with LED branches |
| Prototype PCB | 1 | `Perma-Proto` or generic perfboard | For more permanent internal wiring |
| Enclosure | 1 | plastic `ABS` box | For panel mounting and protection |
| Standoffs / spacers | 4 | `M2.5` or `M3` | According to display and board mounting holes |
| Screws and nuts | set | `M2.5/M3` | For mounting in the enclosure |
| 24 to 26 AWG wire | 1 set | stranded or solid | For internal wiring |
| Connectors / pin headers | as needed | `2.54 mm` | Makes servicing and replacement easier |
| `5 V USB` source | 1 | permanently connected adapter | Final unit power supply |

## Recommended Specific Parts

### `ESP32`

- preferred type for the current prototype: `LOLIN/Wemos S2 Mini`
- alternative: another quality `ESP32` or `ESP32-S2` dev board with `3.3 V` logic and exposed GPIOs

Notes:

- the current firmware is adapted for `LOLIN/Wemos S2 Mini`
- the default `PlatformIO` build target is `lolin_s2_mini`

### OLED

- recommended module: `SSD1306 128x64`, `0.96"`, `I2C`
- prefer a module that reliably works at `3.3 V`

### Encoder

- for the prototype: LaskaKit `Rotacni encoder s tlacitkem a RC s filtrem`, `LA132020`
- prototype parameters: `20 pulses per revolution`, integrated push button, RC noise filter
- for a more finished build: `Bourns PEC11R`
- reason: the RC-filtered module is convenient for fast `ESP32` bring-up, while `Bourns` is mechanically better for a finished device

### Buttons

- prototype: standard momentary `NO` buttons
- final panel: panel mount buttons, ideally `16 mm`

### Status LED

- recommended variant: two-color `red/green` LED
- alternative: illuminated `standby` button with separate `red` and `green` inputs

## Required Vs Optional Parts

### Required For A Working Prototype

- `ESP32`
- `OLED`
- encoder
- 2 buttons
- status LED
- 2 LED resistors
- wires
- power supply

### Optional But Recommended

- prototype PCB
- panel mount buttons
- encoder knob
- enclosure
- standoffs

## Pins And BOM Mapping

Default firmware pinout:

- `GPIO33` -> `OLED SDA`
- `GPIO35` -> `OLED SCL`
- `GPIO7` -> `Encoder A`
- `GPIO9` -> `Encoder B`
- `GPIO5` -> `Source`
- `GPIO11` -> `Standby`
- `GPIO16` -> `LED Red`
- `GPIO18` -> `LED Green`

Wiring details are in [wiring.md](/Users/peny/Development/Projects/boser-remote-control/docs/en/wiring.md).

## Purchasing Recommendations

- do not buy a custom PCB for the first prototype
- start with the existing `LOLIN/Wemos S2 Mini` you already have
- if you are not sure about the mechanics yet, use cheaper generic buttons for the prototype and buy better panel parts for the final enclosure later
- if the device will sit in a metal case, expect weaker `Wi-Fi` and possibly adjust `ESP32` placement

## Reference Parts And Approximate Prices

Below are examples of currently traceable parts that match this design. Prices are indicative only and may vary by distributor, tax, and shipping.

| Item | Example | Approx. |
|---|---|---:|
| OLED `128x64 I2C` | `Adafruit 0.96" SSD1306`, Product ID `326` | `USD 17.50` |
| Prototype encoder | `LaskaKit Rotacni encoder s tlacitkem a RC s filtrem`, `LA132020` | `48 CZK` |
| `16 mm` panel button | `Adafruit 16mm Panel Mount Momentary Pushbutton`, Product ID `1505/1445/...` | `USD 0.95 / pc` |
| Jumper wires | `Adafruit Premium Male/Male 40x6"`, Product ID `758/3142` | `USD 3.95` |
| Prototype PCB | `Adafruit Perma-Proto Quarter-sized 3-pack`, Product ID `589` | `USD 8.50 / 3 pcs` |
| Enclosure | `Hammond 1591` series | price depends on variant and distributor |
| More robust encoder | `Bourns PEC11R` | price depends on exact configuration and distributor |
| `ESP32` dev board | `LOLIN/Wemos S2 Mini` | price depends on distributor |
