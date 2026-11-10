# Frugal-IoT Menu

Frugal IoT - a local menu, driven by buttons, on a load cell scale.

The menu has three parts that are chosen separately (see `src/control/menu.h`):

- **What it holds**: a tree of `Menu`, `MenuLine` and `MenuNumber`, built in `menu.ino`
- **How it is driven**: `Control_Menu_Simple`, with up, down, accept and back keys
- **Where it is shown**: the displays added with `addDisplay()`. This example always uses
  `Menu_Display_Serial`, so it runs on a board with nothing but buttons attached, and adds
  - an LCD with `ACTUATOR_LCD_WANT`
  - the LCD of another node with `CONTROL_MENU_LCD_NODE="esp32-12bc34"` (same org and project)
  - the OLED, on a board that has one
  - the captive portal page, with four buttons, with `CONTROL_MENU_CAPTIVE_WANT`

```
Scale
├── Weight      shows the reading; accept publishes it on menu/weight
├── Tare        sends 0 to loadcell/output
└── Settings
    ├── Calibrate   number, starting from the current reading, sent to loadcell/output
    └── Language    English / Deutsch / Bahasa Indonesia
```

## Wiring

Each button goes between its pin and GND. Holding up or down repeats, and gets faster the
longer you hold it. The pins are set per board in `platformio.ini` (the `build_flags_pins_*`
groups), so change them there.

| | S2 Mini | C3 72x40 OLED | D1 Mini |
|--------|---------|---------|---------|
| Up     | 3       | 3       | D5 (14) |
| Down   | 5       | 4       | D6 (12) |
| Accept | 7       | 10      | D7 (13) |
| Back   | 9       | 7       | none, so each menu ends in a "Back" row |
| HX711 DOUT / SCK | 16 / 18 | 0 / 1 | D2 (4) / D1 (5) |
| LCD or OLED (I2C) | SDA 33, SCL 35 | built in | |

**One button** - define `SENSOR_BUTTON_ONE_PIN` instead of the four, e.g. the board's boot button.
Click for the next row (while editing a number, the next one down), double click to accept, long
press to go back.

**No load cell** - leave out `SENSOR_LOADCELL_DOUTPIN` and `_SCKPIN`, to try the menu on a board
with nothing attached. The weight then shows nothing.
