# CAD návrh krabičky

Parametrický návrh je v [bose_remote_enclosure.scad](/Users/peny/Development/Projects/boser-remote-control/cad/bose_remote_enclosure.scad).

## Co model obsahuje

- `body`: hlavní díl krabičky s integrovaným čelním panelem, dnem, ložem pro `LOLIN/Wemos S2 Mini`, OLED standoffy a zadním otevřeným servisním prostorem
- `rear_lid`: zadní víko na `4x` šroubek s `USB-C` otvorem a otevřeným slotem pro vedení kabelu
- `panel_layout`: tenký kontrolní výřez rozložení čelního panelu
- `assembly`: vizuální sestava s referenčními bloky pro `OLED`, `S2 Mini`, enkodér a LED

## Výchozí mechanické předpoklady

Model je připravený pro:

- `LOLIN/Wemos S2 Mini`
- běžný `0.96" SSD1306 128x64 I2C OLED` modul
- enkodér s panelovým závitem a zadním modulem podobným `LA132020`
- jednu samostatnou stavovou LED se subtilním zapuštěným čelním otvorem `2.2 mm`

Aktuální panel je kompaktní a všechny prvky jsou v jedné svislé ose: `OLED`, enkodér, `LED`.

Pokud se skutečné díly liší, uprav přímo horní parametry v `.scad`.

## Kritické rozměry k přeměření

Před prvním finálním tiskem přeměř:

- `oled_board`, `oled_hole_spacing`, `oled_window`
- `encoder_panel_hole_d` a zadní obrys encoder modulu
- `led_hole_d`, `led_relief_d`, `led_relief_depth`
- `s2_board`, `usb_opening`, `usb_slot_center_z`

## Export

Pokud máš nainstalovaný `OpenSCAD`, typický export je:

```bash
openscad -D 'part=\"body\"' -o body.stl cad/bose_remote_enclosure.scad
openscad -D 'part=\"rear_lid\"' -o rear-lid.stl cad/bose_remote_enclosure.scad
openscad -D 'part=\"panel_layout\"' -o panel-layout.stl cad/bose_remote_enclosure.scad
```

## Doporučení pro tisk

- tisknout `body` čelním panelem na podložce; tím vznikne čistý pohledový čelní panel a zadní strana zůstane otevřená pro montáž
- tisknout `rear_lid` plochou stranou na podložce
- pro rychlé ověření rozložení prvků nejdřív vytisknout `panel_layout`
- tryska `0.4 mm`, vrstva `0.2 mm`, `3-4` perimetry
- materiál `PLA` pro rychlý prototyp, `PETG` pro trvalejší kus
