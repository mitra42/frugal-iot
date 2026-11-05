# Flash size investigation — plan

## Status

**Complete** (reviewed 2026-11-05). Kept as the record of how the numbers were reached; the
practical guide distilled from it is [FLASH_SIZE.md](../FLASH_SIZE.md), and creep is tracked by
`scripts/size_check.py`.

Loose ends below that are now closed, though their checkboxes were not ticked at the time:

- **1.5e** — the LoRaMesher `ostringstream` fix landed upstream; `platformio.ini`'s
  `lib_deps_lora` is back on `https://github.com/loramesher/LoRaMesher.git`.
- **0h** — the weak-reference `printAppData` fix stays, and `SYSTEM_LORAMESHER_DEBUG` is now
  commented out by default in `build_flags_loramesher`.
- **0e / 0f** — the esptool backup is gone; the recovery recipe is kept below.
- **2c** — superseded by the step 1.6 price list (`ds18b20` +9,320, `loadcell` +8,636).
- **Structural findings** — `Frugal-IoT.h` now includes `sensor/gps.h` and `control/carousel.h`,
  and `platformio.ini` documents the RS485 pins `SENSOR_ULTRASONIC_SLAVE_ID` needs.
- "Still open: commit the step-0 changes" — committed.
- **3g** — done 2026-11-05. Nothing of ours to recover: the ~52-59 KB of `printf`/`scanf`
  machinery is anchored by framework code, our float formats are all debug-only, and literals
  are not duplicated. The only lever left is newlib nano format, priced at an estimated ~37-45 KB
  and not pursued. Details in the results file.

Deliberately not done, and not blocking:

- **3c** — `SYSTEM_CAPTIVE_WANT` (~50.5 KB). A product decision, since it would also remove the
  device's configuration web UI. Revisit only if a board gets tight.
- **1.6e** — module prices on a LoRa board. Optional.

---

Goal: **no board is short of flash today** — `min_spiffs` gave headroom. What we are chasing is
*creep*: making sure flash is not being wasted by bad practices that include things we do not
need, or by third-party libraries that pull in more than they should. So the deliverable is as
much a set of standing checks as it is a one-off saving.

Corollary: measurements are worth keeping even when they show something is fine, and anything
found to be big-but-wanted (captive portal, languages) gets a `_WANT`-style switch so it can be
dropped when a board *does* get tight — not removed now.

Reference envs for all measurements — both first-class, since they answer different questions:
- **`c3_pico`** (ESP32-C3, riscv32-esp-elf) — clean baseline, plenty of headroom, 73.3% used.
- **`lilygo_t3_s3_sx127x`** (ESP32-S3, xtensa) — **94.5% used, 108 KB free.** The board that is
  actually tight, and the one where LoRaMesher + RadioLib + SSD1306 + GFX get measured.

Status legend: `[ ]` todo · `[~]` in progress · `[x]` done · `[-]` dropped

---

## Findings so far (step 1 — static confirmation)

Your assumptions are **correct in principle**, and the toolchain is configured the way it needs
to be for them to hold. Evidence:

| Mechanism | Status | Evidence |
|---|---|---|
| `lib/Frugal-IoT` is compiled into a static archive, so the linker pulls in **individual `.o` members only when a symbol from them is needed** | ✅ | PlatformIO `lib_archive` defaults to `True` (`platformio/project/options.py:607`); nothing in `platformio.ini` or `library.json` overrides it |
| Third-party libs (DallasTemperature, OneWire, BH1750, DHTNEW, SHT85/SHT4x, HX711, TinyGPSPlus, ModbusMaster, LoRaMesher…) are likewise separate archives | ✅ | 30 `lib*/` archive dirs under `.pio/build/c3_pico/` |
| Dead code *inside* a pulled-in `.o` can still be dropped | ✅ | `-ffunction-sections -fdata-sections` on every TU + `-Wl,--gc-sections` at link (`platforms/espressif32/builder/frameworks/_bare.py:65`) |
| No central sensor registry / factory table that would reference every sensor | ✅ | `system/discovery.cpp` advertises polymorphically via `frugal_iot.discover()`; no file cross-references other sensors' classes (each `sensor/*.cpp` only names its own) |
| `frugal.cpp` only unconditionally instantiates small things | ✅ | `Sensor_Health`, `Actuator_Ledbuiltin`; `Actuator_OLED` behind `ACTUATOR_OLED_WANT`, `Sensor_Battery` only via `configure_battery()` |
| Building an unused library costs no flash | ✅ | `lib_ldf_mode = chain` decides what gets *compiled*; an archive nobody references is never linked |

So: not adding `Sensor_ENS160` in `main.cpp` should leave `ens160.o` out of the image, and not
adding `Sensor_DS18B20` should leave DallasTemperature and OneWire out entirely. **This is the
part that still needs empirical proof** — see step 2.

### Exceptions already identified by inspection (to be quantified, not yet confirmed as costly)

1. **Project `src/` is NOT archived.** PlatformIO compiles `src/*.cpp` to plain `.o` files and
   passes them straight to the linker, so archive laziness does not apply to
   `src/control_lcd_ht.cpp`, `src/control_oled_gps.cpp`, `src/control_oled_ht.cpp` — they are
   linked whether or not `main.cpp` uses them. `--gc-sections` *should* still discard their
   unreferenced functions and vtables, but this is exactly the case where gc-sections is
   known to leak (retained vtables, `KEEP`'d sections). Worth measuring first.
   *(On `c3_pico` two of the three are behind `ACTUATOR_OLED_WANT`, which is undefined there,
   so use `ttgo` to test this one.)*
2. **File-scope objects with constructors** defeat gc-sections for whatever they reference,
   because their `.init_array` entry is `KEEP`'d. In the library there are exactly two:
   `system/captive.cpp:42-43` — `static DNSServer dnsServer;` and
   `static AsyncWebServer server(80);`. These pull ESPAsyncWebServer + DNSServer in
   unconditionally. Probably intended (captive portal is always wanted) but likely one of the
   largest single contributors, so it needs a number against it.
3. **`LANGUAGE_ALL` is on by default** and `system/language.cpp` fills a global `TT[]` array, so
   *every* translation table is linked. The `_settings.h` comment says the set is EN/NL/DE/ID,
   but the code also carries **FR, SP and HI**. That is 7 string tables of ~30 lines each,
   always present.
4. **Debug flags currently enabled** in `platformio.ini` (`SENSOR_AHT_DEBUG`,
   `SENSOR_ENS160_DEBUG`, `SYSTEM_NESSAGE_DEBUG`, `SYSTEM_WIFI_DEBUG`) add format strings and
   `Serial.print` calls. Needs to be part of the baseline definition so measurements are
   comparable.

---

## Step 0 — unblock the build  ✅ DONE

`pio run -e c3_pico` **failed at HEAD** (library commit `0f6bfa5`, clean tree), for a reason
unrelated to this investigation:

```
lib/Frugal-IoT/src/sensor/ens160.cpp:41: error: 'DEFAULT_ens160_temperature_min' was not declared
lib/Frugal-IoT/src/sensor/ens160.cpp:42: error: 'DEFAULT_ens160_humidity_min' was not declared
```
(and the matching `_max` / `_color` for both)

Root cause: `ens160.cpp` now takes temperature/humidity as *wireable compensation inputs*, but
the `ens160:` module in `modules.yaml` still only lists `aqi/tvoc/eco2/aqi500`. The legacy
`ensaht:` module has the temperature/humidity leaves, which is why `defaults.h` has
`DEFAULT_ensaht_temperature_*` but no `DEFAULT_ens160_temperature_*`.

- [x] **0a.** Added `temperature` and `humidity` leaves to the `ens160:` module in
      `frugal-iot-server/config.d/schema/modules.yaml`, using `leaf_from: controlfloat`
      (`wireable: true`, `rw: w`, `log: false`) rather than the read-only `temperature` /
      `humidity` topics — these are compensation *inputs* wired from another sensor, and the
      sensor that actually measures them is the one that should log them. Same pattern the
      `climate` module already uses. Overrides: temperature `max: 50`, `color: red`;
      humidity `color: blue` — which reproduces the values the legacy `ensaht` module gave.
- [x] **0b.** Propagated with `frugal-iot-server/scripts/copy-schema-to-examples.zsh`
      (3 example `modules.yaml` updated in frugal-iot-logger), regenerated with
      `node scripts/generate-defaults.js`, copied result to `lib/Frugal-IoT/src/defaults.h`.
      Diff is the 6 expected `DEFAULT_ens160_{temperature,humidity}_{color,min,max}` defines,
      plus one unrelated `DEFAULT_captive_language_code_color` that came from `topics.yaml`
      edits already sitting uncommitted in the logger checkout.
- [x] **0c.** `pio run -e c3_pico` now compiles and **links** cleanly.
- [x] **0d.** `lilygo_t3_s3_sx127x` builds — after a *second*, unrelated pre-existing break,
      below.
- [x] **0g.** `ttgo-lora32-v21` confirmed building — 1,670,608 bytes, 85.0%.

### Second pre-existing break: `printAppData()`  ✅ FIXED

`lilygo_t3_s3_sx127x` failed at link with `undefined reference to printAppData()` from three
places in `system/loramesher.cpp`. By design `SYSTEM_LORAMESHER_DEBUG` expects the *application*
to supply that function (`loramesher.cpp:131`, and `examples/loramesher/loramesher.ino` does) —
but `platformio.ini` turns the flag on for **every** LoRa env via `build_flags_loramesher`, so
any sketch that does not write one fails to link. Optional by intent, mandatory by accident, and
with no diagnostic pointing at the cause.

Fixed by making it a weak *reference* — `extern void printAppData() __attribute__((weak));` —
and guarding the three call sites with `if (printAppData)`. Verified in the object file:
`nm` shows `w _Z12printAppDatav`, and the disassembly of `periodically()` has the `beqz a8`
null test skipping the call, so a sketch that does define one is still called normally.

Deliberately **not** an empty weak *definition*: that also links, but the compiler may inline
the empty body at the call sites, and then a sketch supplying its own would silently never be
called. The comment in the source says so, to stop it being "simplified" later.

- [ ] **0h.** Your call whether that is the fix you want, or whether
      `-D SYSTEM_LORAMESHER_DEBUG` simply should not be on by default for every LoRa env.

### The broken `esptool`  ✅ FIXED

`.bin` generation failed with `ModuleNotFoundError: No module named 'esptool.__init__'`, so
nothing could be flashed. Diagnosis: `~/.platformio/packages/tool-esptoolpy/` had become a
mongrel — its `.piopm` claimed version 5.3.0 while its `package.json` and contents were the
ancient flat-layout `1.30000.201119` (a single `esptool.py`, no `esptool/` package directory).
The editable pip install of esptool 5.3.0 in the venv maps `import esptool` to that directory,
so the import failed.

Cause is structural, **so expect it to come back**: the two platforms want different esptools
and install to the same un-suffixed directory name —

| platform | wants |
|---|---|
| `espressif32` (pioarduino 55.03.39) | `tool-esptoolpy` 5.3.0, from a pioarduino release zip |
| `espressif8266` (4.2.1) | `tool-esptoolpy` `~1.30000.0`, from the PlatformIO registry |

whichever built last wins. My earlier `d1_mini` attempt is what broke it this time.

Fix applied: the correct 5.3.0 was still staged in `~/.platformio/tools/tool-esptoolpy`, so I
moved the mongrel aside to `~/.platformio/packages/tool-esptoolpy.broken-backup`, copied the
staged 5.3.0 into `packages/tool-esptoolpy`, and restored a `.piopm` recording 5.3.0.
`pio run -e c3_pico` now builds `firmware.bin` and `firmware.factory.bin` cleanly.

- [ ] **0e.** Delete `~/.platformio/packages/tool-esptoolpy.broken-backup` once you are happy.
- [ ] **0f.** Decide whether to care about the recurrence. **Recovery recipe** if an ESP8266
      build breaks ESP32 flashing again:
      `cp -R ~/.platformio/tools/tool-esptoolpy ~/.platformio/packages/` and re-add the
      `.piopm`. A sturdier option is to stop building ESP8266 envs from this checkout, or to
      keep a separate `PLATFORMIO_CORE_DIR` for them.

**It did recur, and the fuller diagnosis is this:** `~/.platformio/penv/bin/` contains both a
generated `esptool` console script and a copy of `esptool.py`. Because Python puts a script's own
directory first on `sys.path`, a stale flat `penv/bin/esptool.py` **shadows** the installed
`esptool` package for anything run from that directory. It had been replaced by the 186 KB
esptool **3.0** single-file script, so `esptool version` reported 3.0 while
`python -c "import esptool"` correctly reported 5.3.0 — which is why the first diagnosis looked
like a missing package rather than a shadowed one.

Fixed by copying `packages/tool-esptoolpy/esptool.py` (the 1,175-byte 5.3.0 shim) over
`penv/bin/esptool.py`, and re-creating the missing `penv/bin/esptool` console script by hand
(`pip install -e` cannot rebuild it offline). Both now report 5.3.0. The stale v3 file is kept in
the scratchpad. **So the check is `~/.platformio/penv/bin/esptool version` — if it does not say
5.3.0, that shadowing is back.**

### First numbers, and a correction worth knowing about

| | bytes |
|---|---|
| `.flash.text` | 1,084,990 |
| `.flash.rodata` | 208,756 |
| `.eh_frame` | 64,552 |
| DRAM `.text` (IRAM) | 67,420 |
| **`firmware.bin` (what actually gets flashed)** | **1,440,976** |
| PlatformIO's reported "Flash used" | 1,375,935 |
| `app0` partition (`min_spiffs.csv`, `0x1E0000`) | 1,966,080 |
| Real headroom | 525,104 → **73.3% used** (PlatformIO says 70.0%) |

**PlatformIO's "Flash: nn%" line understates the image by ~64 KB, and that matters directly for
watching creep.** The missing piece is `.eh_frame`. It is not a rounding artefact:
`readelf -S` shows `.eh_frame` with the `A` (alloc) flag inside `LOAD` segment 01 alongside
`.flash.rodata`, and `firmware.bin` is 1,440,976 — matching the ELF total, not PlatformIO's
figure. So **use `ls -l firmware.bin`, not the build summary**, for any size tracking.

`.eh_frame` is C++ exception unwind tables. `c3_pico` compiles with **`-fexceptions`** (and
`-fno-rtti`), and the prebuilt framework has `CONFIG_COMPILER_CXX_EXCEPTIONS=y` baked into its
sdkconfig — so most of that 64 KB belongs to the framework, not to us, and `-fno-exceptions` on
our own code only reclaims our share. Getting the rest needs `custom_sdkconfig` to rebuild the
framework. Documented as a commented-out flag in `build_flags_frugaliot`; quantified in step 3f.

**This is RISC-V only.** `t3_s3` (Xtensa ESP32-S3) has **no `.eh_frame` section at all** —
`readelf -S` shows none, and its `firmware.bin` is within 571 bytes of PlatformIO's reported
figure. So of this project's envs only `c3_pico` pays the 64 KB and only `c3_pico` has a
misleading size readout; every Xtensa board (`s2_mini`, `nodemcu-32s`, `ttgo`, `t3_s3`, `tbeam`,
`heltec*`) reports honestly. Worth re-checking per architecture rather than assuming.

*(Aside spotted in the flags: `-std=gnu++2b` and `-std=gnu++2a` are both passed, so the later
one wins. Framework's doing, harmless, just odd.)*

### `t3_s3` — this is the board that is actually tight

`lilygo_t3_s3_sx127x` builds and links, at **1,857,952 of 1,966,080 bytes = 94.5%, leaving
108,128 free**. Unlike `c3_pico` the reported figure is trustworthy here (no `.eh_frame`), so
94.5% is the real number. Same `main.cpp` (the ENS160/AHT21 example) as the c3_pico measurement,
so none of the difference is application logic — it is the LoRa + OLED stack: LoRaMesher,
RadioLib, Adafruit SSD1306 + GFX, plus `Actuator_OLED` and the two `src/control_oled_*.cpp`
files, which `ACTUATOR_OLED_WANT` switches on here and off on `c3_pico`. (That also makes this
the env where exception (1) — the un-archived project sources — can finally be tested.)

| | `c3_pico` | `t3_s3` | delta |
|---|---|---|---|
| `.flash.text` | 1,084,990 | 1,297,308 | +212,318 |
| `.flash.rodata` | 208,756 | 457,912 | +249,156 |
| `.iram0.text` | 67,420 (DRAM) | 77,439 | |
| `firmware.bin` | 1,440,976 | 1,857,952 | **+416,976** |
| headroom | 525,104 | **108,128** | |

That answers the "no board is hurting" question: **`t3_s3` is 5.5% from the wall**, and it is
the LoRa stack that puts it there. Promoting it from secondary env to a first-class target.

The `.flash.rodata` delta (+249 KB, more than doubling) is the striking one — that is data, not
code, so it wants explaining before anything else. Candidates: RadioLib's tables, the SSD1306/GFX
font data, and LoRaMesher's debug strings (`SYSTEM_LORAMESHER_DEBUG` is on for every LoRa env in
`platformio.ini`, and `CORE_DEBUG_LEVEL` / `RADIOLIB_DEBUG_BASIC` are available next to it).

---

## Step 1 — measurement harness  ✅ DONE

`lib/Frugal-IoT/scripts/size_report.py` is written and validated; `${common.build_flags_map}` in `platformio.ini`
gives every env a `firmware.map`. Baselines and findings are in
[FLASH_SIZE_RESULTS.md](FLASH_SIZE_RESULTS.md).

- [x] **1a.** `-Wl,-Map=$BUILD_DIR/firmware.map` added as `build_flags_map`, applied to all envs.
- [x] **1b.** `lib/Frugal-IoT/scripts/size_report.py` — per-library and per-source-file flash, the
      contributed-zero-bytes list, per-library string-literal upper bound, biggest input sections,
      and a reconciliation against the ELF's own section headers that refuses to vouch for the
      numbers when they do not add up. Both envs currently reconcile at +0.
- [x] **1c.** Baseline recorded.

**Step 2 is largely answered as a side effect** — see the results file. Short version: unused
sensor code is *not* costing flash, but the mechanism is `--gc-sections`, not archive laziness,
because the pioarduino platform forces `lib_archive = False`. And the biggest single item found
is 203 KB of C++ iostreams/locale pulled in by LoRaMesher's use of `std::ostringstream`.

<details>
<summary>Original step 1 plan, kept for reference</summary>


Everything below hinges on one linker feature: with `-Wl,-Map=firmware.map`, the map file opens
with a section titled **“Archive member included to satisfy reference by file (symbol)”**. That
is a *definitive, no-guessing* list of which archive members were pulled in and *which symbol
pulled them*. It answers your question directly.

- [ ] **1a.** Add `-Wl,-Map=$BUILD_DIR/firmware.map` to the reference env's `build_flags`
      (or a small `extra_scripts` snippet so it applies to every env)
- [ ] **1b.** Write `lib/Frugal-IoT/scripts/size_report.py` (kept in this repo, not the library) that takes a
      build dir and emits:
      - total `.text` / `.rodata` / `.data` / `.bss` and flash used vs partition size
      - the archive-member inclusion list, grouped by owning library, with per-member size
      - top ~60 symbols by size (`nm -C --size-sort`), tagged with owning object
      - a flat total per library, so "how much is ESPAsyncWebServer costing me" is one line
- [ ] **1c.** Establish the **baseline**: exact `platformio.ini` state (which debug flags on),
      commit hash, and the numbers. Record in `FLASH_SIZE_RESULTS.md`.

</details>

## Step 2 — confirm the assumption empirically  ✅ MOSTLY DONE

Settled by the step 1 report; details and evidence in the results file.

- [x] **2a/2b.** Confirmed: every unused Frugal-IoT sensor object and every unused third-party
      sensor library contributes **0 bytes**. 27 of 58 Frugal-IoT objects dropped entirely.
- [x] **2d.** Assumption holds *in effect*, but via `--gc-sections` rather than archive laziness -
      `lib_archive = False` is forced by the platform "to make weak defs in framework and libs
      possible". That is a more fragile mechanism, which is why the check is worth keeping.
- [ ] **2c.** Differential test (add `Sensor_DS18B20`, measure the delta) still worth doing as a
      sanity check on the tool, and to price a sensor honestly.

## Step 1.5 — 203 KB of iostreams on `t3_s3`  ✅ PRICED at 230,480 bytes

- [x] **1.5a.** Measured, not estimated: **230,480 bytes**, 94.5% → **82.8%**, headroom
      108,128 → 338,608. `patches/loramesher-no-iostreams.diff`, full breakdown in the results
      file. Measurement reverted; baseline byte-identical again.
- [x] **1.5b.** Answered: **no**, `logger.hpp`'s `<iostream>` is not the anchor — it sits behind
      `#ifndef LORAMESHER_BUILD_ARDUINO` and is not compiled on Arduino builds. The anchors are
      the five real stream *instantiations*; an unused `#include` costs nothing.
- [x] **1.5d.** `toolchain libc` answered as a side effect: it fell 110,003 → 67,739 in the same
      change, so its excess was the locale machinery pulling in `printf`/`scanf` support. Not a
      separate problem.
- [x] **1.5c.** Done via option 2. `mitra42/LoRaMesher` already existed as a fork, with a local
      clone at `~/git/github_mitra42/LoRaMesher`, so nothing needed creating. Branch
      `perf/avoid-iostreams` (`44ada81`) off upstream `1abec4a`, pushed; `platformio.ini`
      switched to the fork branch with the upstream URL commented above it. **230,464 bytes
      banked**, verified from the git URL with `.pio/libdeps` deleted first.
      Kept in `lib_deps_lora` rather than vendored into `lib/`, since `lib/` is scanned for every
      env while LoRaMesher belongs only on LoRa boards — and it keeps the one-line revert.
- [x] **1.5e.** PR submitted upstream by you. Maintainer is usually slow but positive; when it
      lands, switch `platformio.ini`'s `lib_deps_lora` back to the upstream URL (the line is
      already there, commented). Until then the fork branch stays.
      Two things a reviewer may raise, both deliberately left out of the PR to keep the diff
      reviewable, and worth offering as a follow-up rather than folding in:
      `utils/logger.hpp`'s `std::cout` (native builds only, so harmless here) and
      `utils/file_log_handler.hpp`'s `<sstream>`/`<iomanip>` (referenced nowhere, so never
      compiled).

<details>
<summary>Original step 1.5 notes</summary>


`toolchain libstdc++` is the largest single contributor on the board with the least room, at
203,761 bytes - the full `std::locale` facet set including wide-char and monetary formatting.
Cause is `std::ostringstream` in six LoRaMesher source files plus `<iostream>` in its
`utils/logger.hpp`; a stream instantiation anchors `std::locale`'s static init, which
`--gc-sections` then cannot unpick.

- [ ] **1.5a.** Confirm by experiment: patch LoRaMesher's `ostringstream` uses out (they look
      like `toString()` helpers) and measure the delta. Do it in `.pio/libdeps` first - throwaway,
      just to price the win before deciding whether to pursue it properly.
- [ ] **1.5b.** Check whether the `<iostream>` in `utils/logger.hpp` alone is enough to anchor it,
      since that header is likely included very widely.
- [ ] **1.5c.** If the win is real, decide the route: upstream PR to LoRaMesher (`snprintf` or
      Arduino `String` instead), or carry a patch. Note `platformio.ini` already tracks
      `https://github.com/loramesher/LoRaMesher.git` directly and there is a commented-out
      `mitra42/LoRaMesher.git#new_loramesher` fork, so carrying a patch is already an option.
- [ ] **1.5d.** Re-check `toolchain libc` too - 110,003 on `t3_s3` vs 62,835 on `c3_pico`. The
      `_vfprintf_r` / `_svfprintf_r` / `__ssvfiscanf_r` objects suggest full `printf` float and
      scanf support being pulled in by something.

</details>

## Step 1.6 — price the other optional modules  ✅ DONE

`lib/Frugal-IoT/scripts/price_modules.py`, results in the results file. 23 modules priced on `c3_pico`.

- [x] **1.6a.** Scripted. Never touches `src/main.cpp` — generated sketches plus
      `PLATFORMIO_SRC_DIR`, and `PLATFORMIO_BUILD_DIR` keeps objects out of the real
      `.pio/build`. Measures the ELF, not `firmware.bin`, so it needs no esptool.
- [x] **1.6b/c.** All the prime suspects priced, plus the no-external-library ones.
- [x] **1.6d.** **Headline: nothing is pathological.** Largest is `gps` at +21,142; there is no
      second LoRaMesher in this set. So no lobbying, forking or rewriting is called for on the
      strength of size — which is the answer that only measuring could give.
- [x] Established that the prices are **not additive**: three analog sensors together cost
      +19,578 against +42,046 summed individually, because the ~12 KB `Sensor_Analog` base is
      charged to each. Siblings in a family are nearly free.

Two structural findings fell out of modules that would not compile: `Frugal-IoT.h` omits
`sensor/gps.h`, `control/carousel.h`, `control/oled.h` and `control/logger.h`; and
`SENSOR_ULTRASONIC_SLAVE_ID` needs `SYSTEM_RS485_RX_PIN`/`_TX_PIN` too or it `#error`s. Both
worth deciding on separately from the size work.

- [ ] **1.6e.** Optional: re-run on `ttgo`/`t3_s3` if a LoRa-board number is ever needed. The
      module costs should be much the same; the interesting figure there was LoRaMesher's, and
      that is banked.

<details>
<summary>Original step 1.6 plan</summary>

Your point, and the natural next step now the harness exists: add each optional sensor / actuator
/ control to `main.cpp` one at a time, rebuild, and record the delta. That gives a price list, so
a decision about any given module is made against a number rather than a guess.

The libstdc++ finding is the argument for doing this systematically rather than by suspicion: it
was invisible until measured, and it was 15% of the image.

- [ ] **1.6a.** Script it — `lib/Frugal-IoT/scripts/price_modules.py` or similar: for each module, generate a
      one-sensor `main.cpp`, build, run `size_report.py --json`, diff against an empty baseline.
      Slow (a couple of minutes per build) but unattended.
- [ ] **1.6b.** Prime suspects, since they bring third-party libraries with them: `Sensor_DS18B20`
      (DallasTemperature + OneWire), `Sensor_LoadCell` (HX711), `Sensor_GPS` (TinyGPSPlus),
      `Sensor_Ultrasonic` (ModbusMaster), `Actuator_LCD` (hd44780), `Control_GSheets` (TLS),
      `Sensor_BH1750`, `Sensor_DHT`, `Sensor_SHT`.
- [ ] **1.6c.** Also price the Frugal-IoT-internal ones with no external library, to catch
      anything pulling `String`/`std::` machinery in unexpectedly.
- [ ] **1.6d.** Record as a price list in the results file, and note which route (lobby / fork+PR
      / rewrite over `TwoWire`) each finding would call for. The I2C sensors are the ones already
      largely rewritten, so a rewrite there is cheap; a rewrite of something like LoRaMesher is
      not.

</details>

## Step 2 (original) — confirm the assumption empirically

- [ ] **2a.** Baseline build of `c3_pico` with `main.cpp` as-is (AHT21 + ENS160). Confirm from
      the map that `ens160.o` and `aht.o` ARE included, and that `ds18b20.o`, `bh1750.o`,
      `dht.o`, `sht.o`, `loadcell.o`, `gps.o`, `ms5803.o`, `bme680.o`, `bmx280.o`,
      `dissolvedoxygen.o`, `ina219.o`, `ultrasonic.o`, `soil.o`, `button.o` are NOT.
- [ ] **2b.** Confirm no member of `DallasTemperature`, `OneWire`, `BH1750`, `DHTNEW`, `SHT85`,
      `SHT4x`, `HX711`, `TinyGPSPlus`, `ModbusMaster` appears in the map at all.
- [ ] **2c.** Differential test: add `Sensor_DS18B20` to `main.cpp`, rebuild, diff the reports.
      The delta is the true marginal cost of that sensor + its two libraries. Revert.
      Repeat for one more (`Sensor_LoadCell` → HX711) as a cross-check.
- [ ] **2d.** If 2a/2b hold → assumption confirmed, and the size problem is elsewhere (core,
      WiFi, web server, always-on library code). If they don't hold → the map says exactly which
      symbol dragged it in; chase that.

## Step 3 — hunt the exceptions  ✅ DONE

All measured; numbers and reasoning in the results file. Summary:

- [x] **3a.** Not needed — `size_report.py`'s zero-byte listing already proves `--gc-sections`
      discards whole objects, and the one thing it does not discard turned out to be `.eh_frame`
      (see 3g2). Nothing left for a `--print-gc-sections` audit to find.
- [x] **3b.** Un-archived project `src/` is not a leak. Settled earlier.
- [~] **3c.** Captive portal priced at ~50.5 KB, but the same `AsyncWebServer` serves the device's
      configuration web UI, so a `SYSTEM_CAPTIVE_WANT` switch would remove both. Product decision,
      not a size cleanup — and less pressing now `t3_s3` has 339 KB free. Left open deliberately.
- [x] **3d.** `LANGUAGE_ALL` → `LANGUAGE_EN` saves only **3,708** bytes. The suspicion that seven
      language tables were significant is **not** borne out. No switch needed.
- [x] **3e.** All four debug flags off saves **2,692** bytes. Minor.
- [x] **3f.** **`-fno-exceptions` saves 75,166 bytes** — and my earlier claim that most of
      `.eh_frame` belonged to the framework was wrong: `.eh_frame` falls 64,552 → 11,476, so ~53 KB
      was ours. **Cannot be global**: LoRaMesher throws in 10+ files and `t3_s3` fails to compile
      with the flag (verified). Per-env for `c3_pico` / `s2_mini` / `nodemcu-32s`.
- [x] **3g2.** The ~6.8 KB Xtensa residue is `.eh_frame` input sections from otherwise fully
      discarded libraries, folded into `.flash.rodata`. Not removable while exceptions are on.
      Also corrects an earlier note: `t3_s3` does carry eh_frame data, just not as its own output
      section.
- [x] **3g.** Float `printf` / `String` / duplicated-literal audit — done 2026-11-05, nothing to
      recover in our code. See the results file.

<details>
<summary>Original step 3 plan</summary>

- [ ] **3a.** `-Wl,--print-gc-sections` build; look for anything large that was *not* discarded
      but is unreachable from `main.cpp`.
- [x] **3b.** Exception (1) settled: `--gc-sections` handles the un-archived project sources.
      `project src` is `[1 of 4 used]` on `c3_pico` and `[3 of 4 used]` on `t3_s3`, totalling
      1,110 bytes there. No leak.
- [ ] **3g2.** NEW: the ~6.8 KB of flash *data* that fully-unused libraries still contribute on
      `t3_s3` but not on `c3_pico` (`DallasTemperature` 1,024, `Hash` 936, `hd44780` 880,
      `Adafruit BusIO` 880, `HX711` 568 …). Xtensa-specific and unexplained. Small, but it is
      the "library includes things unnecessarily" pattern, so worth understanding.
- [~] **3c.** Exception (2) priced: `ESPAsyncWebServer` 33,411 + `AsyncTCP` 9,985 +
      `DNSServer` 2,749 + `AsyncUDP` 4,406 = **~50.5 KB** on `c3_pico`. Real, and worth a
      `SYSTEM_CAPTIVE_WANT` switch for a tight board, but not where the fat is. Still to do:
      confirm how much of that is *only* reachable from the captive portal rather than shared
      with OTA / the web UI, since that is what the switch would actually recover.
- [ ] **3d.** Quantify exception (3): build with `-D LANGUAGE_EN` vs default `LANGUAGE_ALL`.
      **Agreed outcome:** if large, add a way to reduce to English only. Also worth deciding
      whether FR/SP/HI belong in `LANGUAGE_ALL` at all — `_settings.h` documents the set as
      EN/NL/DE/ID, so those three look like they arrived without the comment being updated.
- [ ] **3e.** Quantify exception (4): build with all debug flags commented out.
- [ ] **3f.** `.eh_frame` (64,552 bytes, already confirmed present in `firmware.bin`): build
      with `-fno-exceptions` in `build_flags_frugaliot` and measure the delta, to split "our
      share" from "the framework's share". If the framework's share dominates, evaluate
      `custom_sdkconfig` with `CONFIG_COMPILER_CXX_EXCEPTIONS=n` — bearing in mind the prebuilt
      libs are compiled with it on, so this may not be safely switchable.
      Separately: check whether `std::function` / `std::vector` / `String` instantiations show
      up large in the symbol list.
- [ ] **3g.** Look for float `printf` / `String` formatting pulls, and duplicated string
      literals (`.rodata` is often the surprise).

</details>

## Step 4 — act on it

### 4a. Ranked findings  ✅

| # | finding | worth | status | route |
|---|---|---|---|---|
| 1 | LoRaMesher `std::ostringstream` → locale/iostreams | **230,464** (`t3_s3`) | **banked**, PR submitted upstream | fork + PR |
| 2 | `-fno-exceptions` on non-LoRa envs | **75,166** (`c3_pico`) | measured, **not applied** | one line per env |
| 3 | Captive portal + web UI | ~50,500 | priced, deliberately not pursued | product decision |
| 4 | `.eh_frame` from unused libraries (Xtensa) | 6,808 | explained, not actionable | needs conditional deps |
| 5 | `LANGUAGE_ALL` → English only | 3,708 | measured | not worth a switch |
| 6 | Four debug flags | 2,692 | measured | already per-need |
| 7 | Every optional sensor/control module | +1,982 … +21,142 each | measured, all sane | none needed |

Items 5 and 6 are small enough that they are better left as they are than turned into more
configuration. Item 7 is the reassuring one: nothing among the 23 modules misbehaves.

- [x] **4b/4c.** Applied, using the shared-default-plus-opt-out shape rather than per-env:
      `-fno-exceptions` lives in `[common] build_flags_frugaliot`, and every LoRa env carries
      `build_unflags = ${common.build_unflags_loramesher}` to take it back off.
      Measured: `c3_pico` **−75,168** (73.3% → 69.5%); `t3_s3` and `ttgo` **byte-identical**.
      `build_unflags` *removes* the flag; countering it with `-fexceptions` instead would have
      cost the LoRa boards 6,232 bytes, because `-fexceptions` also adds unwind tables to C
      sources that the default omits. Both measured rather than assumed.
      Failure mode if a new LoRa env forgets the `build_unflags` line is a loud compile error
      (`exception handling disabled, use '-fexceptions' to enable`), not silent breakage.
- [x] **4d.** `lib/Frugal-IoT/scripts/size_check.py` + `size_baseline.json`. Builds the tracked envs,
      compares against the recorded baseline, exits non-zero past a 2% margin (`--margin` to
      change, `--update --note "why"` to re-record). Not put in the library's
      `scripts/prerelease.bash` as originally planned — that script lives in the library, iterates
      the examples and generates `platform.h`/keywords; it builds nothing, so a size check on this
      project's envs does not belong there.
      Current baseline: `c3_pico` 1,365,689 (69.5%), `lilygo_t3_s3_sx127x` 1,627,365 (82.8%),
      `ttgo-lora32-v21` 1,670,478 (85.0%).

## Step 5 — `examples/all` refreshed

Not originally in the plan; added because the pricing work turned up which modules were missing.
`examples/all` is the compile-everything test sketch, so it is the natural place for them, and it
gives future sessions one sketch that exercises most of the library.

- [x] Added `Sensor_BMP280`, `Sensor_BME280`, `Sensor_BME680`, `Sensor_INA219`,
      `Sensor_DissolvedOxygen` (wired to the SHT for temperature compensation — it is the only
      Sensor with an `IN`), `Sensor_GPS` (guarded `#ifdef ESP32`, since the ESP8266 envs here have
      no second UART that can receive), `Sensor_Ultrasonic` (guarded on
      `SENSOR_ULTRASONIC_SLAVE_ID`), `Actuator_LCD` (guarded on `ACTUATOR_LCD_WANT`) and
      `Control_Carousel` (guarded on `ACTUATOR_OLED_WANT`, as it cycles OLED displays).
- [x] Verified by compiling against the **working** library rather than the registry copy the
      example's own `platformio.ini` would pull — `PLATFORMIO_SRC_DIR` pointed at the example from
      this project. Builds on `c3_pico`, `lilygo_t3_s3_sx127x` (exercises the OLED/carousel path)
      and `d1_mini` (ESP8266). The two flag-guarded blocks were built explicitly too, since
      untested code in a compile-test example defeats the point — which caught a wrong
      `System_RS485` constructor: it takes a `HardwareSerial*` and is not a `System_Base`, so it
      must not be added to `frugal_iot.system`.
- [ ] Still absent, and arguably worth adding later: `Sensor_Health` and `Actuator_OLED` are added
      automatically so cannot be added again; `Control_Climate` and the `Control_Oled_*` family
      live in this project's `src/`, not the library.

<details>
<summary>Original step 4 plan</summary>

- [ ] **4a.** Rank findings by bytes-per-unit-of-effort in `FLASH_SIZE_RESULTS.md`.
- [ ] **4b.** For each item worth doing, decide: `_WANT` guard (the pattern the library already
      uses for LCD/OLED/LoRaMesher/mDNS/Modbus/SPI), compile-time flag, or code change.
- [ ] **4c.** Implement highest-value items one at a time, re-measuring after each.
- [ ] **4d.** Add a size-regression check to `scripts/prerelease.bash` so this does not silently
      drift back.

</details>

---

## Answered

1. **Which board hurts?** None right now — `min_spiffs` gave room. The concern is creep, not a
   present emergency. `ttgo`/`t3_s3` still worth asking about separately because LoRaMesher is
   big. *(→ folded into the goal above.)*
2. **Budget?** No hard target. Judge findings by whether they represent waste, not by bytes
   against a deadline.
3. **Captive portal / languages negotiable?** Get numbers first. If large, add
   `SYSTEM_CAPTIVE_WANT` and an English-only option, to be used when a board gets tight.
   *(→ 3c, 3d.)*
4. **Step 0** — fixed properly across the server/logger repos. Done.

## Still open

- Whether to commit the step-0 changes (three repos, plus `loramesher.cpp` and `platformio.ini`)
  now or after the whole investigation.
- **0h** above: is the weak-reference fix the one you want for `printAppData`?

*(`t3_s3` / LoRaMesher is no longer an open question — it is now a first-class target, since
that board is at 94.5%.)*
