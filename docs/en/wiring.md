# Wiring and Connections

This document describes the electrical wiring of the `v1` prototype. The default reference board is `LOLIN/Wemos S2 Mini`. The Bose amplifier is not connected to the `ESP32` by any wire. Communication with the `SA-5` is done exclusively over `Wi-Fi`.

## Full Pinout And Diagram

![Project pinout for LOLIN Wemos S2 Mini and connected components](../assets/wemos-s2-mini-project-wiring.svg)

The image shows the full `LOLIN/Wemos S2 Mini` pinout. Only the pins used by this project carry the extra wiring annotations for the `OLED`, encoder, encoder button, and status LED.

## Basic Principle

- the `ESP32` reads local controls
- the `ESP32` displays status on the `OLED`
- the `ESP32` sends commands to the `Bose SA-5` over the network

That means the only electrical parts on the desk or inside the enclosure are:

- `ESP32` development board
- `OLED SSD1306 128x64 I2C`
- rotary encoder
- encoder push button
- single status LED

## Pin Map

Default pins are defined in [PinConfig.h](/Users/peny/Development/Projects/boser-remote-control/include/PinConfig.h).

| Function | ESP32 pin | Note |
|---|---:|---|
| OLED `SDA` | `GPIO33` | `I2C` data |
| OLED `SCL` | `GPIO35` | `I2C` clock |
| Encoder `A` | `GPIO7` | quadrature input |
| Encoder `B` | `GPIO9` | quadrature input |
| Encoder button `SW` | `GPIO11` | active in `LOW`, also used as service hold during boot |
| Status LED | `GPIO18` | LED output |

## Power

### `LOLIN/Wemos S2 Mini` Variant

- power the `ESP32` from `USB 5 V`
- power the `OLED` from the board `3.3 V`
- buttons and encoder are passive inputs tied to `GND`

### Important Notes

- do not feed `5 V` directly into `GPIO`
- if a specific `OLED` module only supports `5 V` logic, verify compatibility with `3.3 V`
- common `GND` must be connected between the `ESP32`, `OLED`, encoder, and buttons

## Wire Connections

### `SSD1306 I2C` OLED

| OLED pin | Connect to |
|---|---|
| `VCC` | `3.3 V` |
| `GND` | `GND` |
| `SDA` | `GPIO33` |
| `SCL` | `GPIO35` |

## Rotary Encoder

Typical pins are `A`, `B`, `COM`, and sometimes a built-in push button `SW`.

| Encoder pin | Connect to |
|---|---|
| `A` | `GPIO7` |
| `B` | `GPIO9` |
| `COM` | `GND` |
| `SW` | `GPIO11` |

The firmware uses internal pull-ups, so external resistors are not required for a typical mechanical encoder.

## Encoder Button

The built-in encoder push button is wired as a simple switch to ground.

| Contact | Connect to |
|---|---|
| `SW` | `GPIO11` |
| other switch contact | `GND` |

The firmware uses `INPUT_PULLUP`, so the idle state is `HIGH` and a press pulls the input to `LOW`.

## Status LED

The default firmware expects one independently controlled LED.

| LED contact | Connect to |
|---|---|
| `Anode` | `GPIO18` -> dedicated series resistor -> LED |
| `Cathode` | `GND` |

Notes:

- the default firmware logic is `active HIGH`
- if you use another active state, change `STATUS_LED_ACTIVE_HIGH` in [PinConfig.h](/Users/peny/Development/Projects/boser-remote-control/include/PinConfig.h)
- the resistor belongs between `GPIO18` and the LED
- typical values are `220R` to `1k`; `330R` is a sensible prototype default

## Simple Block Diagram

```text
                 +----------------------+
                 |   Bose SoundTouch    |
                 |       SA-5           |
                 |   Wi-Fi only link    |
                 +----------^-----------+
                            |
                         Wi-Fi LAN
                            |
+--------------------------------------------------+
|                      ESP32                       |
|                                                  |
|  GPIO33 <------ SDA -------- OLED 128x64         |
|  GPIO35 <------ SCL -------- OLED 128x64         |
|  3V3    ------> VCC -------- OLED                |
|  GND    ------> GND -------- OLED                |
|                                                  |
|  GPIO7  <------ A ---------- Encoder             |
|  GPIO9  <------ B ---------- Encoder             |
|  GND    ------> COM -------- Encoder             |
|                                                  |
|  GPIO11 <------ Encoder SW -----> GND            |
|  GPIO18 ------> [330R] --> LED anode            |
|  GND    ------> LED cathode                     |
|                                                  |
+--------------------------------------------------+
```

## Recommendations For A Real Build Or Enclosure

- keep `I2C` wires as short as possible
- use clean wiring for buttons and encoder, especially if the wires are longer
- if the device goes into a metal enclosure, verify `Wi-Fi` quality
- place the `ESP32` so the antenna is not shielded by metal

## Not Part Of `v1` Wiring

- no relay for disconnecting Bose power
- no analog audio output from the `ESP32`
- no connection to Bose `AUX` connectors
- no physical modification of the `SA-5` electronics
