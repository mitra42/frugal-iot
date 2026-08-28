# Board matrix

Which physical board each `[env:…]` targets, and what each toolchain calls it.

One row per **physical board**, keyed on `board` + `board_build.variant` + any board-identifying
`-D ARDUINO_*` the env adds — deliberately *not* on the `platform.h` guard, because two different
boards can share a guard (`c3_wedoo` and `r4` both land on `ARDUINO_ESP32C3_DEV`). Envs that differ
only in sensors or options (`_4x`, `_5`, `_oled`, `_sht`, `_tambak`, …) share a row.

Generated from `examples/*/platformio.ini`, the PlatformIO board JSONs and `arduino-cli board
details`, cross-checked against `scripts/generate_platform_h.py` and `scripts/arduino_compile.bash`.
Hand-edit freely — nothing regenerates this.

| Board | `[env:…]` | PlatformIO `board` / `board_build.variant` | PlatformIO `#define` | Arduino board (Tools > Board) / variant | Arduino `#define` |
|---|---|---|---|---|---|
| LOLIN C3 Pico | `c3_pico` | `lolin_c3_mini` / `lolin_c3_pico` | `ARDUINO_LOLIN_C3_MINI`<br>env adds `-D ARDUINO_LOLIN_C3_PICO` | **LOLIN C3 Pico** / `lolin_c3_pico` | `ARDUINO_LOLIN_C3_PICO` |
| LOLIN S2 Mini | `s2_mini`, `s2_mini_5`, `s2_mini_6`, `s2_mini_4x`, `lightwifi_s2_mini` | `lolin_s2_mini` | `ARDUINO_LOLIN_S2_MINI` | **LOLIN S2 Mini** / `lolin_s2_mini` | `ARDUINO_LOLIN_S2_MINI` |
| ESP32C3 Dev Module | `c3_wedoo` | `esp32-c3-devkitm-1` / `esp32c3` | `ARDUINO_ESP32C3_DEV` | **ESP32C3 Dev Module** / `esp32c3` | `ARDUINO_ESP32C3_DEV` |
| TTGO LoRa32-OLED | `ttgo-lora32-v21` | `ttgo-lora32-v21` / `ttgo-lora32-v21new` | `ARDUINO_TTGO_LoRa32_v21new` | **TTGO LoRa32-OLED** / `ttgo-lora32-v21new`<br>Revision = `TTGO_LoRa32_v21new` | `ARDUINO_TTGO_LoRa32_v21new` |
| LilyGo T3-S3 | `lilygo_t3_s3_sx127x`, `lilygo_t3_s3_sx127x_sht` | `lilygo-t3-s3` / `lilygo_t3_s3_sx127x` | `ARDUINO_LILYGO_T3_S3_V1_X` | **LilyGo T3-S3** / `lilygo_t3_s3_sx1262`<br>set Revision to **Radio-SX1276/SX1278** | `ARDUINO_LILYGO_T3S3_SX1262` |
| Heltec WiFi LoRa 32(V3) | `heltec_wifi_lora_32_V3` | `heltec_wifi_lora_32_V3` | `ARDUINO_heltec_wifi_lora_32_V3` | **Heltec WiFi LoRa 32(V3)** / `heltec_wifi_lora_32_V3` | `ARDUINO_HELTEC_WIFI_LORA_32_V3` |
| Heltec WiFi LoRa 32(V3.2) | `heltec_wifi_lora_32_V32` | `heltec_wifi_lora_32_V3` | `ARDUINO_heltec_wifi_lora_32_V3`<br>env adds `-D ARDUINO_heltec_wifi_lora_32_V32` | **Heltec WiFi LoRa 32(V3)** / `heltec_wifi_lora_32_V3` | `ARDUINO_HELTEC_WIFI_LORA_32_V3` |
| T-Beam | `tbeam`, `tbeam_oled` | `ttgo-t-beam` / `tbeam` | `ARDUINO_T_Beam` | **T-Beam** / `tbeam`<br>any Revision | `ARDUINO_TBEAM_USE_RADIO_SX1262` |
| NodeMCU-32S | `nodemcu-32s`, `nodemcu_tambak` | `nodemcu-32s` | `ARDUINO_NodeMCU_32S` | **NodeMCU-32S** / `nodemcu-32s` | `ARDUINO_NODEMCU_32S` |
| LOLIN(WEMOS) D1 mini Pro | `d1_mini_pro` | `d1_mini_pro` | `ARDUINO_ESP8266_WEMOS_D1MINIPRO †` | **LOLIN(WEMOS) D1 mini Pro** / `d1_mini` | `ARDUINO_ESP8266_WEMOS_D1MINIPRO` |
| LOLIN(WEMOS) D1 R2 & mini | `d1_mini`, `d1_mini_4x` | `d1_mini` | `ARDUINO_ESP8266_WEMOS_D1MINI †` | **LOLIN(WEMOS) D1 R2 & mini** / `d1_mini` | `ARDUINO_ESP8266_WEMOS_D1MINI` |
| Heltec WiFi LoRa 32(V4) | `heltec_wifi_lora_32_V4` | `heltec_wifi_lora_32_V4` / `heltec_V4` ‡ | — | **not usable from Arduino IDE** ‡ | — |
| Nologo ESP32C3 Super Mini | `supermini`, `supermini-4x` | `nologo_esp32c3_super_mini` | `ARDUINO_ESP32C3_DEV` | **Nologo ESP32C3 Super Mini** / `nologo_esp32c3_super_mini` | `ARDUINO_NOLOGO_ESP32C3_SUPER_MINI` |
| ESP32 Dev Module | `lilygohigrow`, `esp32` | `esp32dev` / `esp32` | `ARDUINO_ESP32_DEV` | **ESP32 Dev Module** / `esp32` | `ARDUINO_ESP32_DEV` |
| LOLIN C3 Mini + 72x40 OLED | `esp32c3_oled_72x40` | `lolin_c3_mini` / `lolin_c3_pico` | `ARDUINO_LOLIN_C3_MINI`<br>env adds `-D ARDUINO_C3_OLED_72x40` | **LOLIN C3 Mini** / `lolin_c3_mini` | `ARDUINO_LOLIN_C3_MINI` |
| Sonoff Basic (R2) | `r2` | `sonoff_basic` | `ARDUINO_ESP8266_SONOFF_BASIC †` | **not usable from Arduino IDE** | — |
| Sonoff Basic R4 | `r4` | `esp32-c3-devkitm-1` / `sonoff_basicr4` ‡ | `ARDUINO_ESP32C3_DEV` | **not usable from Arduino IDE** ‡ | — |

† PlatformIO's `platform-espressif8266` is not installed here, so these could not be read from a
board JSON. They are the Arduino core's `build.board`, which the PlatformIO definition mirrors, and
they agree with `board_defines` — inference, not measurement.

‡ Variant not in `framework-arduinoespressif32/variants`. `sonoff_basicr4` is defined locally in
`examples/sonoff/variants/`, which only PlatformIO can use (`board_build.variants_dir`); the Arduino
IDE has no equivalent, so those envs cannot be built from it at all. `heltec_V4` exists nowhere.

## The board guards

PlatformIO reads its per-board settings — pins, I2C addresses, OTA suffix, debug flags — straight
out of `platformio.ini`. The Arduino IDE cannot: it has no idea that file exists. So each example
ships the same settings as C, in two generated files:

- **`platform.h`** — read on ESP32, where the core puts the sketch directory on the include path
- **`esp8266/<examplename>.ino.globals.h`** — read on ESP8266, where it does not (that core
  force-includes this file into every translation unit instead). It sits in a subfolder and has to
  be moved up beside the `.ino`; see its `README.md`.

Both are **auto-generated by `scripts/generate_platform_h.py`** (run over every example by
`scripts/prerelease.bash`) and must not be hand-edited — re-run the script after changing a
`platformio.ini`.

Since one file has to cover every board an example supports, each `[env:…]` becomes a block wrapped
in `#ifdef <guard>`, where the guard is a macro the compiler defines only for the selected board.
Picking a board in Tools > Board therefore selects exactly one block, and that env's settings reach
the build. If no block matches, a catch-all `#error` at the end of the file says so rather than
letting the sketch compile on library defaults.

### Why the guard is PlatformIO's spelling, not Arduino's

`platform.h` is only ever read by the Arduino IDE, so Arduino's macro looks like the obvious guard.
It is not, and `_settings.h`'s `TO_ADD_BOARD` block bridges the gap by defining PlatformIO's
spelling from Arduino's *before* `platform.h` is included. Two reasons:

1. **Some boards have several Arduino macros for one PlatformIO board.** LilyGo T3-S3 and T-Beam
   each expose a Board Revision menu where every option sets a different `build.board`
   (`ARDUINO_LILYGO_T3S3_SX1262`, `_SX1276`, `_SX1278`, …). No single Arduino macro covers them;
   collapsing them to one PlatformIO name is what lets a single `#ifdef` per env work.
2. **Everything downstream was written against PlatformIO's names** — the only spelling
   `platformio.ini`, `platform.h` and the library code have to know.

A build for NodeMCU-32S therefore succeeds *because of the alias*, not because Arduino uses mixed
case. **Arduino uses upper case.** Confirm with either:

- `~/Library/Arduino15/packages/esp32/hardware/esp32/<ver>/boards.txt` — `nodemcu-32s.build.board=NODEMCU_32S`
  (line ~19216 in 3.3.11); `platform.txt` line ~152 turns that into `-DARDUINO_{build.board}`
- `arduino-cli board details -b esp32:esp32:nodemcu-32s --show-properties | grep '^build.board='`

Rows whose "Arduino `#define`" is not the guard depend on that alias: NodeMCU-32S and Heltec
V3/V3.2 (case only), T-Beam and LilyGo T3-S3 (different names), LOLIN C3 Pico.

### Where the guard is not the PlatformIO `#define`

Usually it is the same macro, so the table above omits it. Three boards differ, each for its own
reason:

| Board | Guard | PlatformIO `#define` | Why |
|---|---|---|---|
| LOLIN C3 Pico | `ARDUINO_LOLIN_C3_PICO` | `ARDUINO_LOLIN_C3_MINI` | PlatformIO has no C3 Pico board; the env fakes it with an extra `-D`, and the guard uses that, not the board's macro |
| Nologo ESP32C3 Super Mini | `ARDUINO_NOLOGO_ESP32C3_SUPER_MINI` | `ARDUINO_ESP32C3_DEV` | PlatformIO gives it the *same* macro as `esp32-c3-devkitm-1`, so the guard deliberately uses Arduino's unambiguous one and adds no alias |
| Heltec WiFi LoRa 32(V4) | `TODO_HELTEC_WIFI_LORA_32_V4` | — | no board JSON at all; placeholder guard that nothing defines |

## Loose ends

- **`heltec_wifi_lora_32_V4` cannot be built by either toolchain.** No `heltec_wifi_lora_32_V4.json`
  in the installed PlatformIO platform and no `heltec_V4` variant anywhere; no entry in
  `generate_platform_h.py`'s tables or `arduino_compile.bash`'s FQBN map, so its guard is
  `TODO_HELTEC_WIFI_LORA_32_V4`, which nothing defines. Arduino's core *does* have the board
  (`esp32:esp32:heltec_wifi_lora_32_V4`), so that half is one table entry away.
- **`c3_wedoo` and `r4` are different boards sharing the guard `ARDUINO_ESP32C3_DEV`.** Harmless
  today because no example contains both, but an example that did would get one block silently
  dropped — the same trap `board_defines` documents for the SuperMini.
- **`d1_mini_pro` uses Arduino variant `d1_mini`**, not one of its own.
