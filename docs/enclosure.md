# Návrh krabičky

Mechanický návrh je uložený v [cad/bose_remote_enclosure.scad](/Users/peny/Development/Projects/boser-remote-control/cad/bose_remote_enclosure.scad).

## Koncepce

Aktuální verze používá dva hlavní tisknutelné díly:

- `body`: hlavní díl s integrovaným čelním panelem a dnem
- `rear_lid`: servisní zadní víko

Ovládací prvky jsou na skutečně předním panelu:

- `OLED` nahoře uprostřed
- enkodér uprostřed
- jemně zapuštěná stavová `LED` dole uprostřed

`LOLIN/Wemos S2 Mini` je uložená vzadu na lištách a zadní víko má nejen průchod pro `USB-C` konektor, ale i otevřený slot směrem dolů, aby šel kabel vést ven i při zavřeném víku.

## Výchozí rozměry

| Parametr | Hodnota |
|---|---:|
| šířka krabičky | `92 mm` |
| hloubka krabičky | `76 mm` |
| výška krabičky | `54 mm` |
| tloušťka stěny | `2.4 mm` |
| čelní panel | `3.0 mm` |
| spodní stěna | `2.8 mm` |
| zadní víko | `3.0 mm` |

## Rozložení čelního panelu

Souřadnice jsou měřené od levého dolního rohu čelního panelu.

| Prvek | Střed X | Střed Y | Otvor / okno |
|---|---:|---:|---|
| `OLED` okno | `46.0 mm` | `39.0 mm` | `24 x 13 mm` |
| Enkodér | `46.0 mm` | `22.0 mm` | `7.4 mm` |
| `LED` | `46.0 mm` | `8.5 mm` | `2.2 mm` |

## Kontrolní náčrt

```text
Pohled zepředu

  +--------------------------------------------+
  |              [ OLED 24 x 13 ]              |
  |                                            |
  |                   (ENC)                    |
  |                                            |
  |                    o LED                   |
  +--------------------------------------------+
```

## Montážní logika

Montáž je nyní řešená tak, aby šly díly opravdu osadit:

1. vytisknout `body` čelním panelem na podložce
2. zezadu osadit `OLED`, enkodér a LED do integrovaného čelního panelu
3. zavést vodiče dovnitř těla
4. zasunout `LOLIN/Wemos S2 Mini` do zadních lišt
5. zapojit `USB-C`
6. zavřít `rear_lid`

## Co zkontrolovat na reálných dílech

- `OLED` modul musí sedět na rozteč standoffů `23.2 x 23.2 mm`
- vzdálenost `OLED` PCB od panelu musí odpovídat `oled_backoff_from_panel`
- encoder modul musí mít stále dost místa za panelem v oblasti `encoder_relief_d`
- LED pouzdro musí sedět do otvoru `2.2 mm` a do zadního zapuštění `6.8 mm`
- `USB-C` konektor `S2 Mini` musí sedět do výřezu `13.0 x 8.0 mm`
- průměr a výška kabelového límce musí projít spodním slotem zadního víka

## Ověření po prvním tisku

- `OLED` jde přišroubovat zezadu do integrovaného čelního panelu
- prsty mají dost místa kolem enkodéru
- LED je viditelná, ale nepůsobí příliš dominantně proti displeji
- `USB-C` jde zasunout a kabel projde zadním slotem bez páčení desky
