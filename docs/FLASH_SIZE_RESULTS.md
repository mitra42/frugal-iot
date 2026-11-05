# Flash size — measurements

## Status

**Complete** (reviewed 2026-11-05). Measurements are as of 2026-08-22/23 and are not being
updated; `scripts/size_check.py` and its `size_baseline.json` hold the current baseline.

One statement below is out of date: "No PR has been opened upstream yet" under *Banked*. The PR
was opened and has landed, and `platformio.ini` now uses upstream LoRaMesher again. Of the two
structural findings, the `Frugal-IoT.h` includes and the ultrasonic RS485 pins are both
addressed. Step 3g (float `printf` / `String` / literals) was done afterwards, on 2026-11-05.
Open questions: none — see the Status section of
[FLASH_SIZE_PLAN.md](FLASH_SIZE_PLAN.md) for what was deliberately left undone.

---

Companion to [FLASH_SIZE_PLAN.md](FLASH_SIZE_PLAN.md). Numbers only, with the build state they
came from, so later measurements can be compared against them.

Reproduce any of this with:

```
pio run -e <env>
lib/Frugal-IoT/scripts/size_report.py <env> [--detail <library>] [--strings] [--top N]
```

---

## Baseline

| | |
|---|---|
| library commit | `0f6bfa5` + the `printAppData` weak-reference fix + regenerated `defaults.h` |
| `src/main.cpp` | the ENS160 + AHT21 example, unchanged |
| debug flags on | `SENSOR_AHT_DEBUG`, `SENSOR_ENS160_DEBUG`, `SYSTEM_NESSAGE_DEBUG`, `SYSTEM_WIFI_DEBUG`, and `SYSTEM_LORAMESHER_DEBUG` on LoRa envs |
| date | 2026-08-22 |

| env | `firmware.bin` | app partition | used | free |
|---|---|---|---|---|
| `c3_pico` | 1,440,976 | 1,966,080 | **73.3%** | 525,104 |
| `lilygo_t3_s3_sx127x` | 1,857,952 | 1,966,080 | **94.5%** | 108,128 |

### Current, after banking the LoRaMesher fix

`platformio.ini` now builds LoRaMesher from `mitra42/LoRaMesher#perf/avoid-iostreams`.

| env | `firmware.bin` | used | free | vs baseline |
|---|---|---|---|---|
| `c3_pico` | 1,440,976 | 73.3% | 525,104 | unchanged (no LoRaMesher) |
| `ttgo-lora32-v21` | 1,670,608 | **85.0%** | 295,472 | first measurement |
| `lilygo_t3_s3_sx127x` | 1,627,488 | **82.8%** | 338,592 | **−230,464** |

`ttgo` sits 2.2 points above `t3_s3` on the same source — worth a look at some point, but it is
no longer close to the wall.

Always take the size from `firmware.bin`, not from PlatformIO's `Flash: nn%` line — that line
omits `.eh_frame` and so reads ~64 KB low on RISC-V (`c3_pico`). It is accurate on Xtensa, which
emits no `.eh_frame` at all.

### Section totals

| section | `c3_pico` | `t3_s3` |
|---|---|---|
| `.flash.text` | 1,084,990 | 1,297,820 |
| `.flash.rodata` | 208,756 | 457,912 |
| `.iram0.text` | 67,420 | 77,439 |
| `.eh_frame` | 64,552 | — (none) |
| `.dram0.data` | 14,769 | 23,182 |
| of which pooled string literals | 141,318 | 171,551 |

---

## The big one: 203 KB of C++ iostreams + locale on `t3_s3`

`toolchain libstdc++` is the **single largest contributor on `t3_s3` at 203,761 bytes** — larger
than LoRaMesher itself. On `c3_pico` the same library is 10,173 bytes. Per-file:

| object | flash |
|---|---|
| `wlocale-inst.o` | 36,207 |
| `locale-inst.o` | 36,127 |
| `cxx11-wlocale-inst.o` | 18,418 |
| `cxx11-locale-inst.o` | 18,276 |
| `cxx11-shim_facets.o` | 10,220 |
| `cow-shim_facets.o` | 9,998 |
| `sstream-inst.o` | 5,074 |
| `cow-string-inst.o` / `cow-wstring-inst.o` | 4,978 / 4,777 |
| `ostream-inst.o` / `istream-inst.o` | 4,466 / 3,698 |
| `wstring-inst.o` / `string-inst.o` | 3,998 / 3,772 |
| `monetary_members_cow.o`, `lt1-codecvt.o`, `locale_init.o`, … | the rest |

That is the whole `std::locale` facet set — **including the wide-character and monetary
formatting facets**, on a node that has no locale and never touches a `wchar_t`.

**Cause: `std::ostringstream` in LoRaMesher.** Its shipped `src/` instantiates one in five
places, all of them building a diagnostic string:

```
src/types/configurations/pin_configuration.cpp                     PinConfig::Validate()
src/types/configurations/radio_configuration.cpp                   RadioConfig::Validate()
src/types/configurations/loramesher_configuration.cpp              Config::Validate()
src/types/error_codes/result.hpp                                   Result::GetErrorMessage()
src/protocols/lora_mesh/routing/distance_vector_routing_table.cpp  ...::GetStatistics()
```

`utils/logger.hpp` also uses `<iostream>` and `std::cout`, but behind
`#ifndef LORAMESHER_BUILD_ARDUINO`, so it is not compiled here and is **not** part of the
problem. `utils/file_log_handler.hpp` uses `<sstream>` and `<iomanip>` but is referenced nowhere
in the library, so it is never compiled either. The `<sstream>` in
`types/protocols/lora_mesh/network_node_route.cpp` is an unused include and costs nothing — an
include on its own pulls nothing in; it is *instantiating* a stream that anchors the locale.

Instantiating a stream forces `std::locale`'s static init, which anchors every facet — and
because those live in objects reached from a constructor, `--gc-sections` cannot drop them.

**Why this matters most:** it is ~203 KB on the board with 108 KB free. Removing it would nearly
triple that headroom, and it is upstream-fixable (replace `ostringstream` with `snprintf` or
`String`) rather than something we have to live with.

### Priced: **230,480 bytes**, measured not estimated

`patches/loramesher-no-iostreams.diff` replaces the streams with plain string building in all six
files. Measured on `lilygo_t3_s3_sx127x`:

| | before | after | change |
|---|---|---|---|
| `firmware.bin` | 1,857,952 | **1,627,472** | **−230,480** |
| % of app partition | 94.5% | **82.8%** | |
| free | 108,128 | **338,608** | **×3.1** |
| `.flash.text` | 1,297,820 | 1,128,384 | −169,436 |
| `.flash.rodata` | 457,912 | 397,016 | −60,896 |

Where it came from:

| library | before | after | saved |
|---|---|---|---|
| `toolchain libstdc++` | 203,761 | 20,790 | **−182,971** |
| `toolchain libc` | 110,003 | 67,739 | **−42,264** |
| various IDF libs | | | ~−6,500 |
| `LoRaMesher` itself | 151,273 | 152,538 | +1,265 |

Two things worth noting. `libc` gave up 42 KB as well — the locale machinery was pulling in
`printf`/`scanf` support beyond what the firmware itself needs. And LoRaMesher grew slightly,
which is the honest cost of the replacement code.

What remains in `libstdc++` afterwards is ordinary: `cow-string-inst` 3,218,
`string-inst` 2,890, `cow-stdexcept` 1,959, `eh_personality` 1,806, `hashtable_c++0x` 1,399,
`eh_alloc` 1,263 — `std::string`, exceptions, hashing, mutex. The whole locale/iostream block is
gone.

### Banked

Done properly and in the build as of 2026-08-22:

* Branch **`perf/avoid-iostreams`** in `mitra42/LoRaMesher`, commit `44ada81`, branched from
  upstream `1abec4a` — the exact commit the measurement was taken against.
* `platformio.ini`'s `lib_deps_lora` now points at
  `https://github.com/mitra42/LoRaMesher.git#perf/avoid-iostreams`, with the upstream URL left
  commented above it so switching back once a PR lands is a one-line edit. This is the pattern
  the file already used for `#new_loramesher`.
* Verified end-to-end from the git URL after deleting `.pio/libdeps`: **1,627,488 bytes**, and
  `ttgo-lora32-v21` builds too (1,670,608). `c3_pico` is byte-identical, as expected.

The final saving is **230,464** rather than the 230,480 first measured — 16 bytes, from the
tidier `GetStatistics()` (`snprintf` with a provably sufficient 192-byte buffer and
`unsigned long` casts, instead of the throwaway's bare 224-byte buffer).

`patches/loramesher-no-iostreams.diff` is kept as the record of the throwaway measurement; the
real change is the git branch, not the patch file. **Not** applied into `.pio/libdeps` by hand —
that is a build cache PlatformIO can re-fetch at will, and a patch hiding in there would make
local builds silently disagree with a fresh checkout.

No PR has been opened upstream yet.

**Risk assessment for the fork-or-PR decision:** low. All six call sites are diagnostic —
three `Validate()` methods returning human-readable error descriptions,
`Result::GetErrorMessage()`, and `DistanceVectorRoutingTable::GetStatistics()`. None is on the
data path, none is called from an interrupt or a timing-sensitive route. This is close to the
best possible case for option 2 (fork, fix, PR upstream): a large, well-measured saving from a
small, mechanical, easily-reviewed change that Jaime can take or leave without it affecting
protocol behaviour.

---

## The original question: is unused code being linked?

**Answer: no — but not for the reason assumed, and the real mechanism is more fragile.**

The assumption was archive laziness: unused `.cpp` → unused `.o` → never pulled out of the `.a`.
That is not what happens here. **The pioarduino ESP32 platform forces `lib_archive = False`**
unless `platformio.ini` sets it explicitly:

```python
# builder/main.py:952
# Check if lib_archive is set in platformio.ini and set it to False
# if not found. This makes weak defs in framework and libs possible.
if not check_lib_archive_exists():
    projectconfig.set(env_section, "lib_archive", "False")
```

`libFrameworkArduino.a` is the **only** archive in the build. Every `.cpp` of Frugal-IoT and of
every third-party library is handed to the linker as a loose object file, so nothing is skipped
lazily. Unused code is kept out **purely by `-ffunction-sections -fdata-sections` plus
`-Wl,--gc-sections`**.

And empirically that works. On `c3_pico`, contributing **0 bytes**:

- Third-party, entirely dropped: `DallasTemperature`, `OneWire`, `BH1750`, `DHTNEW`, `SHT85`,
  `SHT4x`, `HX711`, `TinyGPSPlus`, `ModbusMaster`, `Button2`, `Hash` (all 4 objects)
- Frugal-IoT, 27 of 58 objects: `sensor/{bh1750,bme680,bmx280,button,dht,dissolvedoxygen,ds18b20,
  gps,ina219,loadcell,ms5803,sht,soil,uint16,ultrasonic}`, `actuator/{lcd,oled}`,
  `control/{carousel,gsheets,logger,logger_fs,oled,oled_loramesher}`,
  `system/{loramesher,mdns,modbus,spi}`
- Arduino core, 29 of 59 objects (USB, touch, ledc, dac, tone, base64 …)

Frugal-IoT's own total is **68,332 bytes on `c3_pico`** — 4.7% of the image. The library is not
the problem; the WiFi/TLS/web stack and (on LoRa boards) the C++ runtime are.

**But the difference matters for the future.** Archive laziness is all-or-nothing per object and
robust. `--gc-sections` is per-section and fragile: anything anchored by a file-scope constructor
keeps its whole object, and everything that object references, alive. The libstdc++ locale
finding above is exactly that failure mode. So the standing check is worth keeping.

### Exceptions from the plan, now settled

1. **Un-archived project `src/`** — *not a problem, but for a different reason than expected.*
   The whole build is un-archived, not just `src/`. And `--gc-sections` handles it: on `c3_pico`
   `project src` is `[1 of 4 used]` with `control_lcd_ht.cpp.o`, `control_oled_gps.cpp.o` and
   `control_oled_ht.cpp.o` all at 0 bytes; on `t3_s3` (where `ACTUATOR_OLED_WANT` is on) it is
   `[3 of 4 used]`, only `control_lcd_ht.cpp.o` dropped, and `project src` totals 1,110 bytes.
2. **`captive.cpp`'s file-scope `DNSServer` / `AsyncWebServer`** — the anchoring is real but the
   cost is modest: `ESPAsyncWebServer` 33,411 + `AsyncTCP` 9,985 + `DNSServer` 2,749 +
   `AsyncUDP` 4,406 = **~50.5 KB** on `c3_pico`. Worth a `SYSTEM_CAPTIVE_WANT` switch for a tight
   board, but it is not where the fat is.
3. **`LANGUAGE_ALL`** — not yet isolated. Frugal-IoT declares 11,261 bytes of string literals
   (upper bound, pre-dedup) across all its surviving objects, so the language tables are at most
   a few KB, not tens. Lower priority than it looked. Still worth the A/B in plan step 3d.
4. **Debug flags** — not yet isolated; see plan step 3e.

### A smaller exemption, real but minor

On `t3_s3` several libraries that are **fully unused** still contribute flash *data* with zero
code — `DallasTemperature` 1,024, `Hash` 936, `hd44780` 880, `Adafruit BusIO` 880, `HX711` 568,
`Button2` 560, `SHT85` 400, `ModbusMaster` 376, `OneWire` 304, `SHT4x` 280, `DHTNEW` 232,
`TinyGPSPlus` 208, `BH1750` 160 — about **6.8 KB** of residue. The same libraries are at a clean
0 bytes on `c3_pico`, so this is Xtensa-specific and not yet explained. Small, but it is the
"library includes stuff unnecessarily" pattern, so worth understanding before trusting
`--gc-sections` completely.

---

## Notes on the tool

`lib/Frugal-IoT/scripts/size_report.py` reconciles its own arithmetic against the ELF section headers and
refuses to vouch for the breakdown if it does not add up. That check earned its place three times
while it was being written, each time by catching a wrong number that looked entirely plausible:

1. **A phantom 137,557-byte section** attributed to `MQTTClient.cpp.o` — which is 1 byte in the
   object file. `ld` pools identical `SHF_MERGE|SHF_STRINGS` sections across the whole link and
   the map credits the entire pool to whichever input section it placed first. Pooled strings are
   now counted as a total and attributed to nobody; `--strings` gives a per-library upper bound
   measured from the object files instead.
2. **1.1 MB of imaginary flash** from counting alignment fill inside `.flash_rodata_dummy` and
   `.dram0.dummy` — `NOBITS` placeholders that reserve MMU address space and hold nothing.
3. **`Adafruit SSD1306` and `Adafruit GFX Library` missing entirely** from the `t3_s3` report,
   because the origin-path regex used `\S+` and those directory names contain spaces. On the
   board being investigated for OLED cost, the OLED libraries were silently absent.

Also: `--strings` initially reported 2.5 MB for Frugal-IoT because `.debug_str` is a mergeable
string section too. It is not `SHF_ALLOC`, so it never reaches the device.

Anyone extending this script: keep the reconciliation honest. A number that is confidently wrong
is worse than no number.

---

## Module price list (`c3_pico`)

From `lib/Frugal-IoT/scripts/price_modules.py`, which builds a bare sketch and then rebuilds it once per module
with just that one module added. Baseline **1,411,729** bytes — framework, WiFi, MQTT, captive
portal and nothing optional. Measured 2026-08-22.

| module | cost | brings with it |
|---|---|---|
| `gps` | **+21,142** | TinyGPSPlus |
| `ultrasonic` | **+20,146** | ModbusMaster, System_Modbus, RS485 |
| `dissolvedoxygen` | +14,910 | *(Sensor_Analog base)* |
| `battery` | +14,680 | *(Sensor_Analog base)* |
| `soil` | +12,456 | *(Sensor_Analog base)* |
| `bme680` | +9,514 | |
| `ds18b20` | +9,320 | DallasTemperature, OneWire |
| `bmp280` | +8,800 | *(Sensor_BMx280 base)* |
| `bme280` | +8,790 | *(Sensor_BMx280 base)* |
| `loadcell` | +8,636 | HX711 |
| `ens160` | +8,432 | |
| `aht21` | +6,878 | |
| `ina219` | +6,432 | |
| `blinken` | +6,212 | |
| `hysteresis` | +6,188 | |
| `lcd` | +5,796 | hd44780 |
| `bh1750` | +5,740 | BH1750 |
| `dht` | +5,540 | DHTNEW |
| `ms5803` | +5,220 | |
| `gsheets` | +5,220 | |
| `sht` | +4,968 | SHT85 |
| `logger_fs` | +3,798 | |
| `carousel` | +1,982 | |

### The headline: nothing is pathological

Every module is priced in the single-digit-to-low-twenties KB range. The largest, `gps` at
21 KB, is a reasonable price for an NMEA parser. **There is no second LoRaMesher here** — nothing
in this set drags in a disproportionate pile of library machinery the way `std::ostringstream` did
at 230 KB. The third-party sensor libraries in particular are all well behaved.

That is worth stating plainly because it was not knowable in advance: the libstdc++ problem was
invisible until measured, so the only way to be confident about the rest was to measure the rest.

### These costs are NOT additive — do not add them up

Each figure is the cost of that module *as the only module*, so a shared base class gets charged
in full to every module that uses it. Demonstrated:

| | cost |
|---|---|
| `battery` + `soil` + `dissolvedoxygen` together | **+19,578** |
| sum of their three individual prices | +42,046 |

The `Sensor_Analog` / `Sensor_uint16` base is ~12 KB of that, counted three times in the table.
The marginal cost of the second and third analog sensor is only about 2.4 KB each. The same
applies to `bmp280` / `bme280` / `bme680`, which share `Sensor_BMx280`.

So read the table as *"what the first sensor of this family costs"*, and expect siblings to be
nearly free. For a real node, measure the actual combination.

### Two structural findings, both from modules that would not compile

1. **`Frugal-IoT.h` does not include `sensor/gps.h`, `control/carousel.h`, `control/oled.h` or
   `control/logger.h`.** Every other sensor and control header is there. So a sketch that
   includes only the umbrella header cannot use `Sensor_GPS` or `Control_Carousel` — it has to
   include the specific header itself. For GPS especially that looks like an oversight rather
   than a decision; worth either adding them or documenting why not.
2. **`SENSOR_ULTRASONIC_SLAVE_ID` alone does not build.** It switches on `SYSTEM_MODBUS_WANT`,
   which then `#error`s unless `SYSTEM_RS485_RX_PIN` and `SYSTEM_RS485_TX_PIN` are also defined.
   That is correct behaviour and the message is clear, but the dependency is only discoverable by
   hitting it — `platformio.ini`'s commented `;-D SENSOR_ULTRASONIC_SLAVE_ID` line does not
   mention the two pins it also needs.

### Caveats on the numbers

* `ms5803` and `gsheets` both come to exactly +5,220. Checked rather than assumed: they are
  genuinely different builds (`.flash.text` 1,069,756 vs 1,069,280, `.flash.rodata` 203,172 vs
  203,480) that happen to total the same. Coincidence, not a stale reading.
* Two modules failed mid-run with an esptool error rather than a compile error. That was the
  toolchain problem recurring, not the modules; both priced fine on a re-run. The script now
  judges success on the ELF existing rather than on `pio`'s exit code, since the ELF is linked
  before the `.bin` step that needs esptool.
* Measured from the ELF's allocated sections, not `firmware.bin`. The two differ by a constant
  ~153-byte header, so deltas are identical, and it avoids esptool entirely.

---

## Step 3 — compiler and configuration flags

Measured on `c3_pico` against an ELF-allocated baseline of **1,440,855** bytes (the real
`src/main.cpp`, not the pricing skeleton), using a scratch build dir so the normal build was not
disturbed.

| change | saving | applies to |
|---|---|---|
| **`-fno-exceptions`** | **−75,166** | non-LoRa boards only — see below |
| `-D LANGUAGE_EN` (instead of `LANGUAGE_ALL`) | −3,708 | all |
| all four debug flags off | −2,692 | all |
| `-fno-exceptions` + `LANGUAGE_EN` together | −78,858 | (additive) |

### `-fno-exceptions` is worth 75 KB — and I was wrong about why

I previously said most of the 64.5 KB `.eh_frame` belonged to the prebuilt framework, so
`-fno-exceptions` on our own code would reclaim only a small share. **That was wrong.** Measured:

* `.eh_frame` falls from 64,552 to **11,476** — so ~53 KB of it is our own code, not the
  framework's, and only 11.5 KB is unavoidable without rebuilding the framework.
* Total saving is **75,166**, i.e. ~22 KB more than the `.eh_frame` change alone. That extra is
  the exception-handling code itself — landing pads, cleanup paths, `__cxa_*` calls.

On `c3_pico` that moves the image from 73.3% of the app partition to about 69.1%.

**But it cannot be applied globally.** `-fno-exceptions` makes `throw` a compile error, and
LoRaMesher throws in at least ten source files. Verified rather than assumed — `t3_s3` with the
flag fails:

```
LoRaMesher/src/utils/byte_operations.h:72: error: exception handling disabled, use '-fexceptions' to enable
```

Frugal-IoT itself uses exceptions in exactly one place, `system/loramesher.cpp:162/246`
(`try`/`catch (const std::exception&)`), which is inside `SYSTEM_LORAMESHER_WANT` — so on a
non-LoRa board nothing in our code or the libraries needs them.

So this is a **per-env flag for `c3_pico`, `s2_mini` and `nodemcu-32s`**, not a global one. Worth
noting the behaviour change is small precisely because nothing catches: with no `try` blocks
outside the LoRa glue, an exception that did escape was already going to abort.

### Languages and debug flags are both minor

`LANGUAGE_ALL` costs 3,708 bytes over English-only, and the four enabled debug flags cost 2,692.
Both are real but neither justifies a `_WANT`-style switch on its own; the earlier suspicion that
the seven language tables were a significant cost is **not** borne out. (The string-literal
upper bound had already hinted at this: Frugal-IoT declares only 11,261 bytes of literals in
total.)

### The Xtensa "residue" explained — it is `.eh_frame` again

The ~6.8 KB that fully-unused libraries contribute on `t3_s3` but not `c3_pico` is, in every
single case, an `.eh_frame` input section folded into the `.flash.rodata` output section:

| library | bytes | | library | bytes |
|---|---|---|---|---|
| `DallasTemperature` | 1,024 | | `ModbusMaster` | 376 |
| `Hash` | 936 | | `OneWire` | 304 |
| `Adafruit BusIO` | 880 | | `SHT4x` | 280 |
| `hd44780` | 880 | | `DHTNEW` | 232 |
| `HX711` | 568 | | `TinyGPSPlus` | 208 |
| `Button2` | 560 | | `BH1750` | 160 |
| `SHT85` | 400 | | **total** | **6,808** |

`--gc-sections` discards all their code and data, but not their unwind tables. So a **correction
to something recorded earlier**: I wrote that `t3_s3` "has no `.eh_frame` at all". That is true of
the *output* sections — there is no `.eh_frame` output section as there is on RISC-V — but the
`.eh_frame` *input* sections are merged into `.flash.rodata` and do occupy flash. The earlier
conclusion that PlatformIO's reported size is accurate on Xtensa still holds, because
`.flash.rodata` is counted.

Not removable while exceptions are on, which on a LoRa board they must be. The only route would
be not compiling those libraries at all — they arrive as `dependencies` in Frugal-IoT's
`library.json`, so every board fetches and compiles every sensor library. 6.8 KB is not worth a
redesign, but it is the reason the figure is not zero.

### Float `printf` / `String` / duplicated literals (step 3g) — nothing of ours to recover

Measured 2026-11-05 on `c3_pico` (1,410,464 bytes, the SHT example) and `lilygo_t3_s3_sx127x`.

**The formatting machinery is all anchored by the framework, not by us.** Both chips' prebuilt
sdkconfig has `CONFIG_LIBC_NEWLIB_NANO_FORMAT` off, so the full newlib `printf`/`scanf`, with
float support, is linked whatever the application does. The map's "Archive member included"
section names the first referrer of each piece:

| libc object | `c3_pico` | `t3_s3` | first pulled in by |
|---|---|---|---|
| `vfprintf` (streams) | 11,154 | 13,421 | IDF `gpio.c` (`fprintf`) |
| `svfprintf` (strings) | 10,876 | 13,125 | Arduino `NetworkInterface.cpp` (`snprintf`) |
| `svfiprintf` (integer-only) | 7,904 | 10,065 | libc `strftime` (`sniprintf`) |
| `svfiscanf` / `svfscanf` | 7,574 | 8,270 | libc `tzset` (`siscanf`), cJSON (`sscanf`) |
| `dtoa`, `mprec`, `gdtoa-*` | 9,230 | 8,452 | via `svfprintf` / `strtod` |
| `strtod` | 3,938 | 3,636 | cJSON |
| **total** | **~52 KB** | **~59 KB** | |

Frugal-IoT is the first referrer of exactly one libc member, `asprintf` (86 bytes, `captive.cpp`).

**Our own float formatting is free.** All five `%f` uses in the library (`control/hysteresis.cpp`,
`sensor/gps.cpp`) are inside `*_DEBUG` blocks, so a release build has none. `String(float, n)`
does not go through `printf` on ESP32: it calls the core's hand-rolled `dtostrf`, 618 bytes.
`String::toFloat()` is `atof` → `strtod`, already linked for cJSON.

**String literals are not duplicated.** Frugal-IoT has 850 distinct literals; only ~1.6 KB is
declared more than once, and `ld` merges identical strings anyway. The largest are deliberate:
the captive portal's HTML/CSS/JS (~1.7 KB) and the 1-Wire wiring diagnostics (100-byte messages,
meant to be read by someone holding the hardware). No shared-prefix families worth factoring out.

**The one remaining lever is not in our code:** `CONFIG_LIBC_NEWLIB_NANO_FORMAT=y`. The C3 and S3
both carry nano `printf` in ROM (`CONFIG_ESP_ROM_HAS_NEWLIB_NANO_FORMAT=y`), so the flash copies
of the three `printf` engines and the scanf one, roughly 37 KB on `c3_pico` and 45 KB on `t3_s3`, could
go. That is an upper bound, not measured. Against it:

* nano drops `%f` from every `printf` in the image, framework included, so it is safe for
  Frugal-IoT release builds but turns our debug output's floats into blanks;
* it needs `custom_sdkconfig`, i.e. pioarduino rebuilding the framework from IDF source, which is
  slow and PlatformIO-only. Arduino IDE users cannot get it at all;
* `dtoa`/`strtod` stay, since cJSON still needs them.

Not pursued: at 69.5% and 82.8% no board needs it. If one ever does, this is the next thing to
measure, and only *after* `-fno-exceptions`.

### Not pursued, with reasons

* **`-Wl,--print-gc-sections` audit** (was 3a): the zero-byte listing in `size_report.py` already
  answers the question it was for — `--gc-sections` is demonstrably discarding whole objects — and
  the one thing it does *not* discard has now been identified as `.eh_frame`. Nothing left to find.
* **Captive portal `_WANT` switch** (3c): priced at ~50.5 KB, but the `AsyncWebServer` instance in
  `system/captive.cpp` serves both the captive portal *and* the device's own configuration web UI.
  Switching it off removes both, so this is a product decision rather than a size cleanup. Worth
  having as an option for a genuinely tight board; not worth spending the UI now that `t3_s3` has
  339 KB free.

---

## Where things ended up

| env | at the start | now | change |
|---|---|---|---|
| `c3_pico` | 1,440,976 (73.3%) | **1,365,689 (69.5%)** | −75,287 |
| `ttgo-lora32-v21` | not building | **1,670,478 (85.0%)** | — |
| `lilygo_t3_s3_sx127x` | 1,857,952 (**94.5%**) | **1,627,365 (82.8%)** | **−230,587** |

(ELF-allocated totals for the "now" column; `firmware.bin` is ~150 bytes more.)

The board that was 5.5% from the wall now has 339 KB of headroom, and the whole saving came from
two changes: LoRaMesher's `std::ostringstream` and `-fno-exceptions` on the boards that can take
it. Nothing else moved the needle by more than about 4 KB.

Tracked from here by `lib/Frugal-IoT/scripts/size_check.py`, which fails past a 2% growth margin.
