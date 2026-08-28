# When flash is tight

What to do when a Frugal-IoT build no longer fits, in the order worth trying. Every number here
was measured on this library, not estimated; the board and date are given so you can tell when one
has gone stale.

Measurements are from 2026-08-23 on `c3_pico` (ESP32-C3) and `lilygo_t3_s3_sx127x` (ESP32-S3),
unless stated.

---

## 0. First, measure it properly

**PlatformIO's `Flash: nn%` line under-reports on the RISC-V boards.** It leaves out `.eh_frame`,
the C++ exception unwind tables, which on an ESP32-C3 is about 64 KB. On `c3_pico` it read 70.0%
when the truth was 73.3%. The Xtensa boards (S2, S3, ESP32) fold `.eh_frame` into `.flash.rodata`,
so their figure is right to within a few hundred bytes.

Use the size of the file that actually gets flashed:

```sh
ls -l .pio/build/<env>/firmware.bin
```

and compare it against your app partition, which is the `app0` line of your partition table:

| `board_build.partitions` | app partition | |
|---|---|---|
| *(default, 4 MB board)* | 1,310,720 | |
| `min_spiffs.csv` | **1,966,080** | +655,360 over the default |
| `huge_app.csv` | 3,145,728 | no OTA — single app slot |

---

## 1. Change the partition table — up to +655 KB

Far and away the biggest single lever, and it costs nothing but filesystem space:

```ini
board_build.partitions = min_spiffs.csv
```

This is what most Frugal-IoT boards already use. It leaves 128 KB of SPIFFS/LittleFS, which is
ample for WiFi credentials and config. If you need no OTA at all, `huge_app.csv` gives 3 MB, but
you lose over-the-air updates — rarely the right trade for a deployed sensor node.

## 2. Turn off C++ exceptions — 75 KB, but not on LoRa boards

Nothing in Frugal-IoT catches an exception except the LoRaMesher glue, so on a board without LoRa
this changes no behaviour:

```ini
build_flags =
    -fno-exceptions
```

Measured on `c3_pico`: **−75,168 bytes**, taking it from 73.3% of a `min_spiffs` partition to
69.5%. About 53 KB of that is `.eh_frame` unwind tables and about 22 KB is the exception-handling
code itself — landing pads, cleanup paths, `__cxa_*` calls.

**LoRaMesher throws**, in a dozen source files, so a LoRa board will not compile with this flag:

```
error: exception handling disabled, use '-fexceptions' to enable
```

If you build both kinds of board from one `platformio.ini`, put `-fno-exceptions` in your shared
flags and take it back off in the LoRa environments with `build_unflags`:

```ini
[common]
build_unflags_loramesher =
    -fno-exceptions

[env:my_lora_board]
build_unflags = ${common.build_unflags_loramesher}
```

Use `build_unflags` to **remove** the flag rather than countering it with `-fexceptions`. Removing
it is free — byte-identical to a build that never had it — whereas `-fexceptions` costs those
boards 6,232 bytes, because on **C** sources it *adds* unwind tables that the default leaves out.

## 3. Add only the sensors you use

Sensors you do not instantiate cost nothing. The linker discards them: on a typical build 27 of
Frugal-IoT's 58 objects and every unused third-party sensor library — DallasTemperature, OneWire,
BH1750, DHTNEW, SHT85, SHT4x, HX711, TinyGPSPlus, ModbusMaster, Button2 — contribute **zero
bytes**. You do not need to edit `library.json` or prune dependencies.

What each one costs when you do add it, on `c3_pico`:

| module | cost | | module | cost |
|---|---|---|---|---|
| `Sensor_GPS` | ~21 KB | | `Sensor_INA219` | ~6.4 KB |
| `Sensor_Ultrasonic` | ~20 KB | | `Control_Blinken` | ~6.2 KB |
| `Sensor_DissolvedOxygen` | ~15 KB | | `Control_Hysteresis` | ~6.2 KB |
| `Sensor_Battery` | ~13-15 KB | | `Actuator_LCD` | ~5.8 KB |
| `Sensor_Soil` | ~12 KB | | `Sensor_BH1750` | ~5.7 KB |
| `Sensor_BME680` | ~9.5 KB | | `Sensor_DHT` | ~5.5 KB |
| `Sensor_DS18B20` | ~9.3 KB | | `Sensor_ms5803` | ~5.2 KB |
| `Sensor_BMP280` / `BME280` | ~8.8 KB | | `Control_Gsheets` | ~5.2 KB |
| `Sensor_LoadCell` | ~8.6 KB | | `Sensor_SHT` | ~5.0 KB |
| `Sensor_ENS160` | ~8.4 KB | | `Control_LoggerFS` | ~3.8 KB |
| `Sensor_AHT21` | ~6.9 KB | | `Control_Carousel` | ~2.0 KB |

**Do not add these up.** Each is the cost of that sensor *as the only one*, so shared machinery is
counted once per row. Three analog sensors together cost 19.6 KB, not the 42 KB the table would
suggest — the second and third are about 2.4 KB each. The same goes for the BMP280/BME280/BME680
family, which share an implementation.

Two things worth knowing about the larger entries:

* **The analog sensors** (`Battery`, `Soil`, `DissolvedOxygen`) look expensive but Frugal-IoT's own
  code is only ~2.5 KB of it. The rest is `analogRead()` pulling in the ESP-IDF ADC driver with its
  calibration and efuse code — about 7.7 KB, paid once however many analog sensors you add.
* **`Sensor_Ultrasonic`** needs three defines together, or the build stops with an `#error`:
  `SENSOR_ULTRASONIC_SLAVE_ID`, `SYSTEM_RS485_RX_PIN` and `SYSTEM_RS485_TX_PIN`.

## 4. Turn off debug flags — ~2.7 KB

The `*_DEBUG` flags in `platformio.ini` each add format strings and `Serial.print` calls. Turning
off all four that a typical build enables saved 2,692 bytes. Worth doing for a release build, but
it will not rescue a build that is badly over.

## 5. Ship one language — ~3.7 KB

The captive portal supports several languages and compiles all of them in by default. Pick one:

```ini
build_flags =
    -D LANGUAGE_EN     ; or LANGUAGE_NL, LANGUAGE_DE, LANGUAGE_ID
```

Measured at 3,708 bytes against the default. Smaller than it sounds like it should be, because the
translation tables are mostly short strings and the linker pools duplicates across the whole image.

---

## Finding out where your flash actually went

These ship with the library, in its own `scripts/` directory:

* `lib/Frugal-IoT/scripts/` if you keep a local copy of the library, or
* `.pio/libdeps/<env>/Frugal-IoT/scripts/` if you pull it from the registry.

They work out which project to look at by walking up from your working directory until they find a
`platformio.ini`, so run them from anywhere inside your project. Substitute your own path for
`<scripts>` below.

**`size_report.py`** reads the linker map and tells you the size of every library and every source
file in the image, plus which files were discarded entirely. First ask the linker for a map:

```ini
build_flags =
    -Wl,-Map=$BUILD_DIR/firmware.map
```

then:

```sh
<scripts>/size_report.py <env>                        # per-library totals
<scripts>/size_report.py <env> --detail Frugal-IoT    # per source file
<scripts>/size_report.py <env> --top 40               # biggest individual symbols
<scripts>/size_report.py <env> --strings              # string-literal bulk per library
```

It reconciles its own arithmetic against the ELF section headers and refuses to vouch for the
numbers if they do not add up, so you can tell a real finding from a misread map.

**`size_check.py`** builds your environments and fails if any has grown more than 2% since a
recorded baseline — worth running before a release, so growth is noticed while it is still one
change ago. The baseline is `size_baseline.json` at the root of *your* project:

```sh
<scripts>/size_check.py                               # check against the baseline
<scripts>/size_check.py --update --note "why"         # re-record after an intended change
```

**`price_modules.py`** measures what each optional sensor, actuator and control costs on your
board, by building a bare sketch and then rebuilding it once per module. That is where the table
above came from; re-run it if you want the numbers for your own board rather than a C3.

```sh
<scripts>/price_modules.py -e <env> --markdown
```

It never touches your `src/main.cpp` — it generates sketches in a scratch directory and points the
build at them, and it keeps its object files out of your `.pio/build`.

---

## A worked example: 230 KB hiding in a third-party library

Worth reading if your build is much larger than the sum of its parts looks like it should be.

A LoRa board was at 94.5% of its partition. The largest single contributor turned out not to be
LoRaMesher or the radio driver but **`libstdc++`, at 203,761 bytes** — the entire C++ `std::locale`
facet set, including wide-character and monetary formatting, on a device with no locale that never
touches a `wchar_t`.

The cause was five uses of `std::ostringstream` in LoRaMesher, all building diagnostic strings.
Instantiating any C++ stream anchors `std::locale`'s static initialisation, and because those
objects are reached from a constructor the linker's `--gc-sections` cannot discard them. Replacing
the streams with plain string building saved **230,464 bytes** and took the board to 82.8%.

The general lesson: `--gc-sections` removes unused code very effectively, but **anything anchored
by a file-scope constructor survives, along with everything it references**. If your image is
mysteriously large, look for a static object pulling in a subsystem you never use. The
`--detail` mode above is how to find it.

*(Fixed in `mitra42/LoRaMesher` branch `perf/avoid-iostreams`; a PR is open upstream.)*

---

## Not available (yet)

* **Switching off the captive portal / web UI.** Worth about 50 KB — `ESPAsyncWebServer` 33.4 KB,
  `AsyncTCP` 10 KB, `DNSServer` 2.7 KB, `AsyncUDP` 4.4 KB. There is **no `SYSTEM_CAPTIVE_WANT`
  flag**: `System_Captive` is constructed unconditionally and `captiveLines()` is a virtual that
  every module implements, so making it optional is a real refactor rather than a `#ifdef`. It
  would also remove the device's own configuration web UI, not just the captive portal, leaving
  WiFi credentials to be supplied through the `data/wifi/` files. Ask if you need it.
* **Rebuilding the framework without exceptions.** Around 11 KB of `.eh_frame` remains after
  `-fno-exceptions` because the prebuilt Arduino/IDF libraries are compiled with
  `CONFIG_COMPILER_CXX_EXCEPTIONS=y`. Changing that needs `custom_sdkconfig` and a framework
  rebuild, which is a lot of machinery for 11 KB.
* **Dropping unused libraries' unwind tables.** On Xtensa, libraries that are otherwise entirely
  discarded still leave about 6.8 KB of `.eh_frame` in `.flash.rodata`. Not removable while
  exceptions are on, and on a LoRa board they must be.
