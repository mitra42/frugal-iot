# Frugal-IoT Library

A platform for affordable, easily built sensor networks running on ESP32/ESP8266 microcontrollers.
Library version: 0.1.3. MIT licence. Author: Mitra Ardron.

Wiki: https://github.com/mitra42/frugal-iot/wiki
Repo: https://github.com/mitra42/frugal-iot

## Compatibility

Every example `.ino` file must work in **both**:
- **Arduino IDE** — open the `.ino` directly; dependencies installed via Library Manager
- **PlatformIO** — referenced via `lib_deps = Frugal-IoT@^0.1.3` in `platformio.ini`

Each example directory contains a `platform.h` file alongside the `.ino`. This file is
**auto-generated** from the example's `platformio.ini` by running `scripts/prerelease.bash`
(which calls `scripts/generate_platform_h.py`). It converts `-D FLAG=value` build flags into
`#define` statements and wraps board-specific defines in `#ifdef ARDUINO_BOARD_NAME` guards.

- **PlatformIO** reads flags directly from `platformio.ini`; `platform.h` is not used.
- **Arduino IDE** picks `platform.h` up because `_settings.h` includes it when `PLATFORMIO` is
  undefined — the `.ino` does not need to (and on ESP8266 could not; see below).

Do not hand-edit `platform.h` — regenerate it by re-running `scripts/prerelease.bash` after
changing `platformio.ini`.

### Test-compiling an example for Arduino

`scripts/arduino_compile.bash <example> <env>` compiles an example exactly as the Arduino IDE
would, from the command line via `arduino-cli` (same cores, libraries and sketchbook as the IDE —
nothing needs opening). Run it from the `scripts` directory:

```
cd lib/Frugal-IoT/scripts
./arduino_compile.bash commonroom nodemcu_tambak
./arduino_compile.bash --list          # every example and its environments
./arduino_compile.bash --install-deps  # core + library.properties deps (ESP32 core is >1GB)
```

It passes `--library ..` so the **working tree** is compiled, not whatever stale copy sits in
`~/Documents/Arduino/libraries`. It regenerates `platform.h` first, and checks two things a bare
"exit 0" would hide:

- **Extra `.cpp` in the sketch folder that also defines `setup()`.** Arduino compiles *every*
  source file in a sketch directory, so that is a duplicate-symbol link error.
- **That the `ARDUINO_*` macro `platform.h` guards the env with is the one the core really
  defines.** If not, the whole `#ifdef` block is skipped, none of the env's flags reach the build,
  and the sketch compiles with library defaults — green, but meaningless. The expected macro is
  obtained by importing `generate_platform_h.py`, so the two cannot drift apart.

`generate_platform_h.py`'s guards must therefore be the **Arduino** macro, not the one PlatformIO
passes as `-D`. Find it with:

```
arduino-cli board details -b <fqbn> --show-properties | grep '^build.board='   # guard is ARDUINO_<that>
```

Its `env_defines` (checked before `board_defines`) exists for envs whose Arduino board differs from
their PlatformIO one — `c3_pico` is `board = lolin_c3_mini` in PlatformIO, which has no C3 Pico
definition, but Arduino has a real `esp32:esp32:lolin_c3_pico` defining `ARDUINO_LOLIN_C3_PICO`
natively. Some boards also need a menu option: TTGO LoRa32-OLED picks its variant through
`Revision=TTGO_LoRa32_v21new`, and without it you silently get the V1 default.

### Partitions: getting min_spiffs in the Arduino IDE

Why this is needed: these boards' default partition scheme (`default.csv`) gives **1,310,720
bytes** of app space, and a full Frugal-IoT build is around 1.36 MB — so it overflows with
*"text section exceeds available space in board"*, which does not obviously point at partitions.
`min_spiffs.csv` gives **1,966,080 bytes** and, importantly, still has both `app0` and `app1`,
so **OTA keeps working**.

The menu is **Tools > Partition Scheme**, and the option, where it exists, is labelled exactly:

> **Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)**

**But most of this project's boards do not offer it.** The core ships `min_spiffs.csv` and 264
board definitions expose it, yet only two of ours do:

| Board (Arduino name) | Used by env | `Minimal SPIFFS` in the menu? |
|---|---|---|
| ESP32C3 Dev Module | `c3_wedoo` | **yes** — just select it |
| LilyGo T3-S3 | `lilygo_t3_s3_*` | **yes** — just select it |
| LOLIN C3 Pico | `c3_pico` | no |
| LOLIN C3 Mini | — | no |
| LOLIN S2 Mini | `s2_mini*` | no |
| NodeMCU-32S | `nodemcu_tambak` | no |
| TTGO LoRa32-OLED | `ttgo-lora32-v21` | no |
| Heltec WiFi LoRa 32(V3) | `heltec_wifi_lora_32_V3*` | no |
| T-Beam | `tbeam*` | no |

#### How this is shipped

Every example directory contains a copy of **`min_spiffs.csv`**, taken from the ESP32 core
(and byte-identical to PlatformIO's copy, so both toolchains use the same table). It is
deliberately named `min_spiffs.csv` and **not** `partitions.csv`, because a file called
`partitions.csv` would override the menu for *every* board — including the 8 MB Heltec boards,
whose `default_8MB.csv` already gives ~3.3 MB and is fine as-is. Left under its own name it is
inert (the IDE does not compile `.csv` files), and the user activates it only if they need it.

Precedence comes from the core's `platform.txt`: *"first and higher priority overwrites it:
build.partitions < variant < source"* — a `partitions.csv` in the sketch directory beats the menu.

#### Text for the user documentation / wiki

> **If compiling gives "text section exceeds available space in board"**
>
> Your board's default flash layout does not leave enough room for this sketch. Two steps:
>
> 1. **Sketch > Show Sketch Folder**, and rename `min_spiffs.csv` to `partitions.csv`.
> 2. **Tools > Partition Scheme >** choose **"No OTA (2MB APP/2MB SPIFFS)"**.
>
> Compile again and it will fit.
>
> Despite that menu option's name, **over-the-air updates still work.** Step 1 is what sets the
> real flash layout, and it keeps both OTA slots. The menu choice only raises the size limit the
> IDE checks your sketch against — every other option on these boards caps it too low to allow a
> sketch this size, even though it fits.
>
> Shortcut: if your board's Partition Scheme menu already lists **"Minimal SPIFFS (1.9MB APP with
> OTA/128KB SPIFFS)"**, just select that and skip step 1 entirely.
>
> If you never see the error, do nothing — boards with more flash (such as the 8 MB Heltec V3)
> have room with their default settings.

One caveat for us rather than the user: with "No OTA (2MB)" selected the IDE's limit
(2,097,152) is slightly above the real `app0` (1,966,080), so a sketch between those two sizes
would pass the check yet not fit. Worth watching the reported percentage.

#### The script does this for you

`arduino_compile.bash` needs neither step and writes nothing into the example directory. It
prefers the menu option when the board has one, and otherwise overrides both properties
directly, reading the app size from the largest `app` partition in the core's own CSV:

```
--build-property build.partitions=min_spiffs --build-property upload.maximum_size=1966080
```

### ESP8266: `<sketch>.ino.globals.h`, and why it sits in a subfolder

The `platform.h` mechanism relies on a library header (`_settings.h`) being able to
`#include "platform.h"` from the *sketch* directory. That works on ESP32 only because its
`compiler.cpreprocessor.flags` carries `"-I{build.source.path}"`. **ESP8266's does not** — so
there `platform.h` is unreachable from library sources, and in fact from the `.ino` too. The
`#include` is wrapped in `__has_include` for exactly that reason, so it does not break the build;
it just silently contributes nothing.

The one mechanism that does reach library sources on ESP8266 is
**`<sketch>.ino.globals.h`**: the core copies it into the build and `-include`s it into every
translation unit. `generate_platform_h.py` emits the ESP8266 `[env:]` blocks into it (measured:
force-included into all 48 library TUs). ESP32 has no equivalent — its `build_opt.h` is a flat
compiler response file and cannot hold `#ifdef` — which is why both files exist.

**But that filename hides the example from the Arduino IDE.** arduino-cli — which Arduino IDE 2
embeds — refuses to recognise a folder as a sketch if it contains *any* file named
`<sketch>.ino*` besides the sketch itself, and the example then disappears from **File >
Examples** with no error anywhere. Verified on arduino-cli 1.5.1:

| File alongside `soil.ino` | Listed in `lib examples`? |
|---|---|
| `soil.h`, `soil.cpp`, `globals.h`, `soil.globals.h` | yes |
| `other.ino.globals.h` | yes |
| `soil.ino.globals.h`, `soil.ino.h`, `soil.ino.txt` | **no** |
| `esp8266/soil.ino.globals.h` (subfolder) | yes |

The name is not negotiable at either end: the core resolves
`globals.h.source.fqfn={build.source.path}/{build.project_name}.globals.h` and `build.project_name`
includes the `.ino`, so `soil.globals.h` is ignored outright (measured: 0 TUs).

So the generated file is parked in the example's **`esp8266/` subfolder**, with a `README.md`
explaining that an ESP8266 IDE user must move it up beside the `.ino` — one drag, no rename, since
the filename is already exact. ESP32 and PlatformIO users ignore the folder entirely.

Left unmoved, an ESP8266 IDE build would otherwise succeed on the library's built-in defaults
instead of the example's settings — silently, since on ESP8266 nothing the example ships is
reachable at compile time until that file is in place. So the generated file carries a
`#define FRUGAL_IOT_GLOBALS_FOUND` outside every `#ifdef`, and **`system/frugal.cpp` `#error`s
without it** on `ESP8266 && !PLATFORMIO`. That marker deliberately means "the file got here", which
is a different failure from "your board is covered" — the latter is the `FRUGAL_IOT_BOARD_CONFIGURED`
catch-all at the end of the same file.

The check lives in a `.cpp`, not in `_settings.h`: the ESP8266 core force-includes into ~48
translation units, so a header would print it 48 times. `SYSTEM_OTA_PREFIX` looks like a tempting
thing to test instead, but OTA is optional by design (`frugal.h` and `ota.cpp` compile it out
cleanly when either half is undefined), so requiring it would break legitimate no-OTA builds.

Someone writing their own ESP8266 sketch and happy with the defaults satisfies the check with a
one-line `<sketch>.ino.globals.h` containing just that define — the same file they need the moment
they want to configure anything.

`arduino_compile.bash` is unaffected: it puts the sketch dir back on the include path by borrowing
the empty `compiler.c/cpp.extra_flags` slots, a lever an IDE user does not have.

## Directory Structure

```
Frugal-IoT/
├── Frugal-IoT.h           # Main include — include this and nothing else in user code
├── src/
│   ├── _settings.h        # Compile-time defaults and guards
│   ├── defaults.h         # Default values for all settings
│   ├── misc.h/cpp         # Shared helpers (e.g. StringF)
│   ├── system/            # Infrastructure: frugal (System_Frugal), wifi, mqtt, ota, power,
│   │                       #   fs, i2c, spi, time, watchdog, base, group, io, message, discovery…
│   ├── sensor/            # One file pair per sensor type (sht, dht, ht, soil, battery, bh1750,
│   │                       #   loadcell, ds18b20, ms5803, ens160aht21, button, analog, float,
│   │                       #   uint16, health, gps, sensor [base class]…)
│   ├── actuator/          # LED, digital output, OLED, LCD (actuator.h is the base class)
│   └── control/           # Logic blocks (hysteresis, logger, logger_fs, blinken, carousel,
│                           #   oled, oled_loramesher, gsheets, control.h [base class]…)
├── examples/              # One subdirectory per example application
│   ├── sht30/             # Temperature + humidity with optional OLED
│   ├── climate/           # Dual-channel hysteresis control (heating + humidifier relays)
│   ├── loadcell/          # HX711 weight scale
│   ├── soil/              # Soil moisture
│   ├── loramesher/        # LoRa mesh gateway + node
│   ├── agri/              # Agricultural multi-sensor
│   ├── all/               # All sensors demo
│   ├── blinken/           # LED blink patterns
│   ├── datalogger/        # LittleFS data logging
│   ├── ensaht/            # ENS160 air quality + AHT21
│   ├── gps/               # GPS location via NMEA serial module + OLED
│   ├── gsheets/           # Google Sheets integration
│   ├── lcd_ht/           # HD44780 LCD showing a remote HT (e.g. SHT) node's readings over MQTT
│   ├── lilygohigrow/      # Plant watering (LilyGo HiGrow)
│   ├── ms5803/            # MS5803 pressure sensor
│   ├── power/             # Power mode demonstration
│   ├── remotedisplay/     # OLED showing a remote SHT node's readings over MQTT
│   └── sonoff/            # Sonoff relay module
└── test/
```

Each example directory contains a `.ino` file (the application) and a `platform.h` (hardware
pin/address overrides), plus — for examples with ESP8266 environments — an `esp8266/` subfolder
holding the equivalent `<sketch>.ino.globals.h` that an ESP8266 Arduino IDE user moves up beside
the `.ino`. See "ESP8266: `<sketch>.ino.globals.h`" above for why it cannot just live there.

## Component Architecture

Frugal-IoT uses four component groups managed by `System_Frugal`:

| Group | Class prefix | Purpose |
|-------|-------------|---------|
| `frugal_iot.sensors` | `Sensor_*` | Read hardware, publish values |
| `frugal_iot.actuators` | `Actuator_*` | Drive hardware outputs |
| `frugal_iot.controls` | `Control_*` | Logic: transform/route signals |
| `frugal_iot.system` | `System_*` | Infrastructure (WiFi, MQTT, OTA…) |

All components inherit from `System_Base` — `Sensor` via the intermediate `System_SensorActuator`,
`Actuator` and `Control` directly. `System_Group` is a separate `System_Base` subclass used as a
*container*: `frugal_iot.sensors`, `frugal_iot.actuators`, `frugal_iot.controls` and `frugal_iot.system`
are each a `System_Group` holding a list of components and forwarding `setup()`/`loop()`/`dispatch()`
to each member.

## System_Frugal API

`System_Frugal` is the one global object every application creates:

```cpp
System_Frugal frugal_iot("org", "project", "device_id", "Human Name");
```

| Method | Call order | Notes |
|--------|-----------|-------|
| `configure_battery(pin)` | Before `pre_setup()` | Optional; adds battery sensor |
| `configure_power(type, cycle_ms, wake_ms)` | Before `pre_setup()` | Sets sleep strategy |
| `pre_setup()` | After battery/power, before everything else | Starts serial, reads filesystem config |
| `configure_mqtt(host, user, pass)` | After `pre_setup()` | MQTT broker connection |
| `sensors->add(new Sensor_*(…))` | After `pre_setup()` | Register sensors |
| `actuators->add(new Actuator_*(…))` | After `pre_setup()` | Register actuators |
| `controls->add(new Control_*(…))` | After `pre_setup()` | Register controls |
| `setup()` | Last in `setup()` | Initialises all registered components |
| `loop()` | Only call in `loop()` | Drives the whole system |

**`loop()`** does **not** block; return quickly. The watchdog resets if `loop()` stalls.

## Power Modes

```cpp
frugal_iot.configure_power(type, cycle_ms, wake_ms);
// Device is awake for wake_ms, then sleeps (cycle_ms - wake_ms) in chosen mode
```

| Type | Notes |
|------|-------|
| `Power_Loop` | Always awake (debugging, relays, frequent readings) |
| `Power_Light` | ESP32 light sleep |
| `Power_Modem` | Modem sleep (minimal saving) |
| `Power_Deep` | Deep sleep — slow to reconnect; use cycle_ms ≥ 60 000 |

### Timing across deep sleep

Deep sleep is a full chip restart — only the RTC domain (RTC_SLOW_MEM/`RTC_DATA_ATTR`, and the RTC
hardware counter) stays powered. Two clocks look similar but behave very differently across it:

- `millis()` / `esp_timer_get_time()` — **reset to 0** on every deep-sleep wake (their counters live
  in the digital domain, which loses power). `esp_timer_get_time()` only stays continuous across
  *light* sleep, not deep sleep — there is no IDF-version exception to this.
- `gettimeofday()` (`sys/time.h`) — anchored to the RTC domain, so it **keeps advancing** across deep
  sleep (and any reset except a full power-on).

`System_Power::sleepSafeSecs()`/`sleepSafeMillis()` (`system/power.h`/`power.cpp`) wrap
`gettimeofday()` for exactly this reason — use them (or `timer_set()`/`timer_expired()`, which are
built on them) for any interval that needs to survive deep sleep, never raw `millis()`. This bit a
previous AI session, which wrote `sleepSafeSecs()` around `esp_timer_get_time()` with a comment
claiming it was "already compensated for deep sleep" — it wasn't; fixed 2026-07-20.

### Using a sleep-safe timer in a component

Any `System_Base` subclass (sensor/actuator/control/system) that needs to do something every N
seconds — but only that often, and correctly even across deep sleep — uses the timer slots on
`frugal_iot.powercontroller` (`System_Power`, `system/power.h`). Pattern (see `system/ota.cpp`,
`system/discovery.cpp`, `system/watchdog.cpp`, `system/time.cpp` for real examples):

```cpp
// 1. Acquire — once per component instance, in the constructor init-list. Do not call timer_next()
//    anywhere else - there are only TIMER_LENGTH (8) slots process-wide (system/power.cpp), and
//    each call permanently claims one for the lifetime of the device.
MyThing::MyThing()
: System_Base("mything", "My Thing"),
  timer_index(frugal_iot.powercontroller->timer_next())
{ }

// 2. Test + set — typically in infrequently() (see below for why), not periodically() or loop().
void MyThing::infrequently() {
  if (frugal_iot.powercontroller->timer_expired(timer_index)) {
    // ... do the infrequent work ...
    frugal_iot.powercontroller->timer_set(timer_index, MYTHING_INTERVAL_S); // re-arm N seconds out
  }
}
```

- `timer_next()` returns an index into an `RTC_DATA_ATTR` array, so the armed time survives deep
  sleep. A freshly-acquired timer defaults to 0, i.e. already expired — it fires on the first check
  unless you call `timer_set()` up front to delay that.
- `timer_expired(i)` compares against `sleepSafeSecs()`, so it works correctly regardless of sleep
  mode; `timer_set(i, secs)` arms it for `secs` seconds from now.
- **Where to call it from**: `periodically()` runs once every wake cycle unconditionally; `infrequently()`
  also runs once per cycle, but is where a component checks its own `timer_expired()` to self-throttle
  to a longer interval than the wake cycle itself. Put timer-gated logic in `infrequently()`, not `loop()`
  (`loop()` runs every pass round `System_Frugal::loop()`, i.e. far more often than the sleep/wake cycle).
- **When you do NOT want this**: for short, sub-cycle backoff/retry timing that only needs to matter
  while the device is already awake (e.g. `wifi.cpp`'s connect-retry backoff, `mqtt.cpp`'s reconnect
  loop), just use plain `millis()` directly — that's what those files do, with a comment noting
  `// Not sleepSafeSecs as this is frequent`. Reaching for a sleep-safe timer there would be wrong,
  not just unnecessary — it would also burn one of the 8 scarce timer slots.
- If you need a raw sleep-safe timestamp rather than the pre-built expiry-slot mechanism (e.g. to
  measure an elapsed duration), call `frugal_iot.powercontroller->sleepSafeSecs()` /
  `sleepSafeMillis()` directly instead.

## IO Classes (IN / OUT) — how sensors, actuators and controls actually connect

Every value a component reads or writes is a member object, not a plain field — an `IN` (input)
or `OUT` (output), both defined in `system/io.h`. This is the mechanism the rest of this doc calls
"signal wiring": `wireTo()`, MQTT publish, and `dispatch()` are all implemented on `IO`, not
hand-rolled per component.

```
IO (system/io.h)          — sensorId, id, name, topicTwig ("sht/temperature"), color, wiredPath…
├── IN                    — subscribes wiredPath on the message bus when wireTo() is called
│   ├── INfloat           — value + min/max/width, e.g. a temperature reading or setpoint
│   ├── INbool
│   ├── INuint16
│   ├── INcolor
│   └── INtext
└── OUT                   — publishes to its own topic AND pushes to wiredPath when set()
    ├── OUTfloat
    ├── OUTbool
    ├── OUTuint16
    └── OUTtext
```

Which group a component uses depends on its role:

| Base class | Holds | Example |
|------------|-------|---------|
| `Sensor` (`sensor/sensor.h`) | `std::vector<OUT*> outputs` | `Sensor_HT` has `OUTfloat* temperature; OUTfloat* humidity;` |
| `Actuator` (`actuator/actuator.h`) | `std::vector<IN*> inputs` | `Actuator_Digital` has `INbool* input` |
| `Control` (`control/control.h`) | both `inputs` and `outputs` | `Control_Hysteresis` — 4 `IN`s (now/greater/limit/hysteresis), 1 `OUTbool` (out) |

A concrete `IN`/`OUT` is constructed with `(sensorId, id, name, ..., color, wireable)` — `id` becomes
the trailing path segment (`topicTwig = "<sensorId>/<id>"`, e.g. `sht/temperature`), and `wireable`
controls whether the UX offers rewiring it at all.

**How wiring and dispatch actually flow:**

1. `wireTo(path)` on an `IN` calls `frugal_iot.messages->subscribe(path)` — it does **not** talk to
   the other `IO` directly. `OUT::wireTo()` just records `wiredPath`; it does not subscribe (that
   would create a useless loopback since an `OUT` never receives).
2. When an `OUT` changes (e.g. `((OUTfloat*)temperature)->set(21.5)`), `set()` both `send()`s to its
   own topic (`sht/temperature`) and `sendWired()`s directly to `wiredPath` — locally looped back if
   `wiredPath` is on this node, or published remotely via MQTT otherwise.
3. Every incoming `System_Message` is cascaded top-down: `System_Frugal` → the relevant `System_Group`
   (`sensors`/`actuators`/`controls`) → each component's `dispatch()` → each of its `IN`/`OUT`
   objects' `dispatch()`. `Control::dispatch()` (`control/control.cpp`) is the canonical example: it
   runs every `output->dispatch(msg)` (handles `.../wired` topic changes), then every
   `input->dispatch(msg)` (handles `.../wired`, `.../value`, `.../min`, `.../max`, `.../cycle`
   suffixes via the typed subclass's `convertAndSet()`), and if any input actually changed value it
   calls `act()` — the method a `Control` subclass overrides to react (e.g.
   `Control_Hysteresis::act()` reads `inputs[0..3]->floatValue()`/`boolValue()` and calls
   `((OUTbool*)outputs[0])->set(...)`).
4. `id`/`sensorId` matching happens inside `dispatch()` itself (`msg.module() == sensorId`), so it's
   safe to call `dispatch()` on every `IN`/`OUT` for every message — most just no-op.

**Wiring one component to another** — either at construction/setup time in code, or later at runtime
via MQTT (`.../wired` topic), or from the captive portal / LittleFS config:

```cpp
// Wire a sensor output to a control input (path() returns this node's own topic path):
cc->inputs[0]->wireTo(sht->temperature->path());

// Wire a control output to an actuator's "set" topic (setPath creates a writable endpoint):
cc->outputs[0]->wireTo(frugal_iot.messages->setPath("heating/on"));
```

Paths follow the pattern `<device_id>/<leaf>`. `setPath` creates a writable endpoint; `path` creates
a readable one — both just build the topic string, the actual subscribe only happens via `IN::wireTo()`.

## Available Sensors

| Class | File | Measures |
|-------|------|---------|
| `Sensor_HT` | sensor/ht | Base class for temp+humidity sensors (`OUTfloat* temperature/humidity`) — not instantiated directly |
| `Sensor_SHT` | sensor/sht | Temperature + humidity (SHT30/SHT40/SHT85); extends `Sensor_HT` |
| `Sensor_DHT` | sensor/dht | Temperature + humidity (DHT11/22); extends `Sensor_HT` |
| `Sensor_Soil` | sensor/soil | Soil moisture (capacitive) |
| `Sensor_Battery` | sensor/battery | Battery voltage |
| `Sensor_BH1750` | sensor/bh1750 | Light (lux) |
| `Sensor_BME280` | sensor/bme280 | Temperature + humidity + pressure (hPa); extends `Sensor_HT`. Freestanding, no external library |
| `Sensor_LoadCell` | sensor/loadcell | Weight via HX711 |
| `Sensor_DS18B20` | sensor/ds18b20 | 1-Wire temperature |
| `Sensor_MS5803` | sensor/ms5803 | Pressure + temperature |
| `Sensor_ENS160AHT21` | sensor/ens160aht21 | Air quality + temp/humidity |
| `Sensor_Button` | sensor/button | Button press events |
| `Sensor_Analog` | sensor/analog | Raw ADC |
| `Sensor_INA219` | sensor/ina219 | Current/voltage/power monitor — shunt (mV), bus (V), current (mA), power (mW), load (V). Freestanding, no external library |
| `Sensor_DissolvedOxygen` | sensor/dissolvedoxygen | Dissolved oxygen (mg/L) from an analog probe, temperature compensated; extends `Sensor_Analog`. **The only Sensor with an `IN`** |
| `Sensor_Float` | sensor/float | Arbitrary float value |
| `Sensor_UInt16` | sensor/uint16 | Arbitrary uint16 value |
| `Sensor_Health` | sensor/health | Device health metrics |
| `Sensor_GPS` | sensor/gps | GPS location (lat/lon/altitude/speed/course/hdop/satellites/UTC time) via NMEA serial module |
| `Sensor_Ultrasonic` | sensor/ultrasonic | Distance (mm) from an RS485/Modbus ultrasonic module (A01ANY4B); needs `SENSOR_ULTRASONIC_SLAVE_ID` |

### System_I2C helpers

`System_I2C` is a plain class (not `System_Base`), one instance **per addressed device**,
holding `addr` plus a `TwoWire*` for the shared bus. Prefer these over hand-rolling:

| Method | Use |
|--------|-----|
| `initialize()` | `wire->begin(I2C_SDA, I2C_SCL)`, **de-duplicated per bus** — every device on a bus calls it from its own `setup()` |
| `sendRegister(reg, value)` | Write one byte to a register |
| `sendRegister16(reg, value)` | Write a big-endian 16-bit register |
| `send1read(cmd, bytes)` | Send a register/command byte, read N bytes back as a big-endian integer (N ≤ 4) |
| `send1read1(cmd)` | The one-byte case |
| `sendAndRead(reg, buf, len)` | Send a register byte, read `len` bytes into a buffer |
| `isPresent()` | Does anything ACK at this address? Cheap wiring check before any chip-specific id read |
| `scan()` | Print every address that ACKs **on this object's bus** |

`sendRegister`, `sendRegister16`, `send1read` and `isPresent` were added after finding the same
code hand-rolled in several sensors: `Sensor_ensaht::ENSsend2()` and `Sensor_BME280::writeReg()`
were byte-identical implementations of the register write, and `ms5803.cpp` paired
`send()`+`read()` five times where `send1read()` now does it. `initialize()` gained the per-bus
guard because every I2C sensor called `wire->begin()` — see the "unnecessary since already
called" note in `ens160aht21.cpp` and TODO-115/TODO-16 in `sht.cpp`. `scan()` previously scanned
the global `I2C_WIRE` regardless of which bus the object was on.

### Sensor_INA219

Freestanding over `System_I2C` — no external library. Five outputs, so it extends `Sensor`
directly (the `Sensor_GPS` shape) rather than `Sensor_Float`. `load = bus + shunt/1000`.

Register layout, LSB scalings (shunt 10 µV/bit signed, bus 4 mV/bit after `>>3`, power LSB =
20 × current LSB) and the calibration arithmetic (`current_LSB = maxCurrent/32768`,
`cal = 0.04096/(current_LSB × shunt)`) were cross-checked against `RobTillaart/INA219` (MIT)
rather than written from memory.

**The calibration trap.** `current` and `power` are meaningless unless
`SENSOR_INA219_SHUNT_OHMS` matches the resistor fitted to the board, and the failure is
**silent** — `shunt` and `bus` stay perfectly correct while `current` and `power` are wrong by
the ratio of the two resistances. Common breakouts fit 0.1 Ω (the default); high-current boards
fit 0.002 Ω, which would read 50× off against it.

```ini
build_flags =
    -D SENSOR_INA219_WANT            ; main.cpp's per-board switch
    -D SENSOR_INA219_SHUNT_OHMS=0.1  ; MUST match the board
    -D SENSOR_INA219_MAX_CURRENT=3.2 ; A; sets resolution (LSB = MAX_CURRENT/32768)
    ;-D SENSOR_INA219_ADDRESS=0x40 -D SENSOR_INA219_CONFIG=0x3FFF -D SENSOR_INA219_DEBUG
```

Bus-register bit 0 is the chip's own math-overflow flag: when set, `shunt` and `bus` are still
published but `current`/`power` are skipped and a message suggests raising `MAX_CURRENT`.

**Power.** Converting continuously costs ~1 mA — for a battery node, more than the thing being
measured. So the chip is left in MODE `000` (power-down, ~6 µA) and woken only for the instant
it is read: write MODE `011` (triggered) → poll the bus register's conversion-ready bit → read
the four registers → back to MODE `000`. Define `SENSOR_INA219_CONTINUOUS` for the old
always-on behaviour, trading ~1 mA for an instant read instead of a ~136 ms blocking wait (at
the default 128-sample averaging on both channels).

This is the pattern to copy for any sensor with a low-power mode:

- **`powerDown()`** (reached via `Sensor::prepare()` before sleeping) writes MODE `000`
  **before** calling the base, so the I2C write happens while the chip still has power.
- **`powerUp()`** (via `Sensor::recover()`) calls the base and then only sets a
  `needs_config` flag. It deliberately does **not** write registers, because
  `System_Power::recover()` runs its `SYSTEM_POWER_ON_DELAY` *after* every sensor's
  `powerUp()` has returned — an I2C write from inside `powerUp()` can hit a chip whose supply
  is still ramping. The flag is acted on at the next read, and rewrites the calibration
  register as well as the config so it is correct whether or not the board actually cut power.

Note the triggered mode helps in **every** power mode, not just sleep:
`System_Power::prepare()` is guarded by `if (mode)`, so under `Power_Loop` nothing ever calls
`prepare()`/`recover()` and a continuously-converting chip would draw its ~1 mA forever.

`SENSOR_INA219_CONFIG` (default `0x3FF8`) holds only the range/gain/averaging bits — the low 3
MODE bits are owned by the class and masked off whatever you pass, so a datasheet-literal
`0x3FFF` also works.

### Sensor_DissolvedOxygen — and how to give a Sensor an input

Extends `Sensor_Analog`, enabled by passing a pin (`SENSOR_DO_PIN` in the demo). Publishes mg/L.

**It maps onto `Sensor_Analog` without overriding `convert()`.** The DO formula is
`DO = voltage_mv * DO_saturation(T) / V_saturation(T)` — pure multiplication — and
`Sensor_Analog` already publishes `(reading - offset) * scale`. So:

- `readInt()` returns the probe voltage in **millivolts**, not raw counts
- `offset` is 0
- `scale` is recomputed as `DO_saturation(T) / (V_saturation(T) * 1000)` whenever a new water
  temperature arrives (the `/1000` converts the table's µg/L to mg/L)

Note the method names: `Sensor_Analog` replaces `Sensor_Float`'s chain with **int** versions —
`readInt()`, `validate(int)`, `convert(int)` — so `readFloat()` is not involved at all.

**This is the first `Sensor` with an `IN`**, and there are three traps if you add another:

1. `Sensor` has only `std::vector<OUT*> outputs` — no `inputs`. The input is a plain member.
2. `Sensor::dispatch()` wraps everything in `if (msg.module() == id)`, but a *wired* input
   receives messages published by a **different** module. So the input's `dispatch()` must be
   called **outside** that test, before delegating upward — exactly what `Control::dispatch()`
   does, and the reason it can't simply be delegated.
3. Hold it as `IN*`, not `INfloat*` — `INfloat::dispatch()` and `::discover()` are protected
   overrides, reachable only through the public `IN` declarations. Also call `input->setup()`
   and add it to `discover()`, neither of which `Sensor` does for you.

The input wires itself in `setup()` to `SENSOR_DO_TEMPERATURE_PATH` (default
`"ds18b20/ds18b20"`) **only if** nothing already wired it, so a path stored on LittleFS or set
in the UX takes precedence over the compile-time default.

```ini
build_flags =
    -D SENSOR_DO_PIN=34
    ;-D SENSOR_DO_CAL1_V=269 -D SENSOR_DO_CAL1_T=25   ; single-point calibration
    ;-D SENSOR_DO_CAL2_V=... -D SENSOR_DO_CAL2_T=...  ; define BOTH for two-point
    ;-D SENSOR_DO_TEMPERATURE_PATH=\"ds18b20/ds18b20\"
    ;-D SENSOR_DO_DEBUG
```

`captiveLines()` is overridden to show the reading read-only: `Sensor_Float::captiveLines()`
offers it as an editable number, which on a `Sensor_Analog` means `calibrate()` and would set a
`scale` that the next temperature message immediately overwrites.

### Sensor_BME280

Extends `Sensor_HT` (which already supplies `temperature` and `humidity`) and adds a
`pressure` output in hPa. Freestanding over `System_I2C` — **no external library** — in the
same spirit as `Sensor_ms5803`.

The compensation arithmetic and calibration unpacking are ported from Bosch's own reference
driver, `boschsensortec/BME280_SensorAPI` (`bme280.c`, `BME280_DOUBLE_ENABLE`), which is
BSD-3-Clause and therefore compatible with this library's MIT licence — the attribution
notice at the top of `bme280.h`/`.cpp` is the requirement. Port rather than reimplement,
because `dig_h4`/`dig_h5` sign-extend the MSB *before* shifting
(`(int16_t)(int8_t)reg_data[3] * 16`), which is easy to get subtly wrong and yields
plausible-but-incorrect humidity.

`double` not `float`: the pressure polynomial divides by constants up to `2147483648.0`,
beyond a 24-bit float mantissa. It runs once per read cycle, so the cost is irrelevant.

Reads in Bosch's **weather monitoring** configuration — forced mode, 1× oversampling on all
three channels, IIR filter off — so the chip sleeps between reads, which suits one reading
per wake cycle. `setup()` resets the device, requires chip id `0x60` (a BMP280 answers `0x58`
and has no humidity, so it cannot sit under `Sensor_HT`) and calls `setupFailed()` otherwise.
Each read also rejects an all-`0xFF` data block, because Bosch's compensation clamps to the
rated range and would otherwise silently publish 85 °C for a disconnected device.

Altitude is deliberately **not** published — it is a re-expression of pressure against an
assumed sea-level reference, better derived downstream from `bme280/pressure`.

```ini
; platformio.ini — the class is always compiled; this flag is main.cpp's per-board switch
build_flags =
    -D SENSOR_BME280_WANT
    ;-D SENSOR_BME280_ADDRESS=0x77   ; default 0x76; 0x77 if SDO is tied high
    ;-D SENSOR_BME280_DEBUG
```

```cpp
frugal_iot.sensors->add(new Sensor_BME280("BME280"));
// or: new Sensor_BME280("BME280", 0x77, &I2C_WIRE, true)
```

Verification: the port was cross-checked against Bosch's functions on the host over 200,000
randomized calibration/raw-value combinations — calibration unpacking, 20/16-bit raw
assembly, `t_fine`, and all three compensated outputs were bit-identical.

### Modbus over RS485 (`system/modbus.h`)

Two plain classes — not `System_Base` subclasses — split the same way `System_I2C` is split
from the `TwoWire` bus it is handed:

| Class | Represents | Owns |
|-------|-----------|------|
| `System_RS485` | One physical connection: a UART plus its half-duplex transceiver | rx/tx pins, DE/RE pins, baud, and the single `ModbusMaster` |
| `System_Modbus` | One addressed slave on that bus | slave id, `connected` flag, retry backoff |

RS485 is multi-drop, so several `System_Modbus` (different slave ids) share one
`System_RS485`. The slave id is re-bound before each transaction, which is cheap —
`ModbusMaster::begin()` only sets `_u8MBSlave`/`_serial` and leaves the callbacks alone.
Keeping one `ModbusMaster` per bus rather than per device also saves RAM: each instance
carries two `uint16_t[64]` buffers, 256 bytes.

A sensor holds its `System_Modbus` **by value** and builds it from `(slave_id, bus)` in its
constructor — compare `Sensor_ms5803`'s `System_I2C interface;`. `System_RS485::initialize()`
is idempotent, so every device on the bus can safely call it from its own `setup()`.

Enabled by `SYSTEM_MODBUS_WANT`, which `_settings.h` derives from any sensor that needs it
(currently `SENSOR_ULTRASONIC_SLAVE_ID`). Bus flags: `SYSTEM_RS485_RX_PIN` and
`SYSTEM_RS485_TX_PIN` (both required — `#error` otherwise), `SYSTEM_RS485_DE_PIN` (0xFF =
transceiver auto-switches direction), `SYSTEM_RS485_RE_PIN` (0xFF = tied to DE),
`SYSTEM_RS485_BAUD` (9600), `SYSTEM_MODBUS_RETRY_CYCLES` (10), `SYSTEM_MODBUS_DEBUG`.

**Timing.** `ModbusMaster::ku16MBResponseTimeout` is `static const uint16_t = 2000` —
compile-time, no setter — so a slave that does not answer blocks `loop()` for a full 2 s.
Fine for a device that is really there (the watchdog is 180 s), but it would be paid every
cycle for one that is absent. `System_Modbus` therefore tracks `connected`: after a failure
it skips the next `SYSTEM_MODBUS_RETRY_CYCLES` read attempts outright, then tries once more.
A device powered up later is picked up automatically, at one 2 s stall per 10 cycles rather
than one per cycle.

### Sensor_Ultrasonic

Reads one Modbus holding register from an ultrasonic distance module over RS485. Enabled by
defining `SENSOR_ULTRASONIC_SLAVE_ID` (the module's Modbus address, usually 1) — that also
turns on `SYSTEM_MODBUS_WANT`. Without it the sensor, both bus classes and `ModbusMaster`
contribute no symbols to the firmware.

```ini
; platformio.ini
build_flags =
    -D SYSTEM_RS485_RX_PIN=16
    -D SYSTEM_RS485_TX_PIN=17
    ;-D SYSTEM_RS485_DE_PIN=26   ; omit if the transceiver auto-switches direction
    ;-D SYSTEM_RS485_RE_PIN=25   ; omit if RE is tied to DE
    -D SENSOR_ULTRASONIC_SLAVE_ID=1
    ;-D SENSOR_ULTRASONIC_REGISTER=0x0101 -D SENSOR_ULTRASONIC_DEBUG
```

```cpp
// One bus object per transceiver, shared by every Modbus device on it
System_RS485* rs485 = new System_RS485(&Serial2);

// Raw distance to the surface, in mm:
frugal_iot.sensors->add(new Sensor_Ultrasonic("ultrasonic", "Distance", 7500, "blue", true, rs485));

// Depth of water instead, for a sensor mounted 2000mm above the tank floor:
frugal_iot.sensors->add(new Sensor_Ultrasonic("depth", "Depth", 2000, "blue", true, rs485, 2000.0, -1.0));
```

The published value is `offset + raw * scale`, where `raw` is the register value in mm.
`offset` and `scale` are persisted to LittleFS and settable over MQTT; `offset` is also
editable in the captive portal (`scale` is not — `addNumber` emits `step=1`, so a
fractional scale cannot be typed in). A failed read returns `NAN`, which
`Sensor_Float::validate()` rejects, so nothing is published for that cycle.

The `HardwareSerial*` is a constructor argument rather than a `#define` because it is a
C++ object, not a number — and note that ESP32-C3/S2 have no `Serial2`.

**To add another Modbus sensor**: subclass `Sensor_Float` (or whichever base fits), give it a
`System_Modbus` member built from `(slave_id, bus)`, call `modbus.initialize()` in `setup()`,
and make `readFloat()` a `modbus.readRegister(reg, &raw)` call. Then add its enabling flag to
the `SYSTEM_MODBUS_WANT` derivation in `_settings.h`.

## Available Actuators

| Class | File | Notes |
|-------|------|-------|
| `Actuator_LEDBuiltin` | actuator/ledbuiltin | Built-in LED; added automatically on supported boards |
| `Actuator_Digital` | actuator/digital | Any digital output (relay, LED) |
| `Actuator_OLED` | actuator/oled | SSD1306 OLED; added automatically on supported boards |
| `Actuator_LCD` | actuator/lcd | HD44780 LCD via I2C backpack; requires `ACTUATOR_LCD_WANT` |

### Actuator_LCD

Drives an HD44780-compatible character LCD via a PCF8574 I2C backpack. Uses `I2C_WIRE` with
auto-detected I2C address. Enable with `-D ACTUATOR_LCD_WANT`.

```cpp
// Enable in platformio.ini:
//   build_flags = -D ACTUATOR_LCD_WANT
// For a 20x4 display (default is 16x2):
//   build_flags = -D ACTUATOR_LCD_WANT -D ACTUATOR_LCD_COLS=20 -D ACTUATOR_LCD_ROWS=4

frugal_iot.actuators->add(new Actuator_LCD());

// Wire a two-line message to it (lines separated by ASCII newline):
frugal_iot.messages->setPath("lcd/message");
// Or wire from another component's output:
someControl->outputs[0]->wireTo(frugal_iot.messages->setPath("lcd/message"));
```

The `message` input accepts a `String`; lines are split on `\n` (ASCII 10). Lines longer than
`ACTUATOR_LCD_COLS` are silently truncated. The display is cleared on every update.

## Available Controls

| Class | File | Notes |
|-------|------|-------|
| `Control_Hysteresis` | control/hysteresis | Single-channel on/off with deadband |
| `Control_Blinken` | control/blinken | LED blink pattern generator |
| `Control_Carousel` | control/carousel | Cycles through a list of child `Control*`s, selected via an `INuint16` |
| `Control_OLED` | control/oled | Base class for custom OLED displays |
| `Control_Oled_LoRaMesher` | control/oled_loramesher | `Control_OLED` subclass showing LoRa mesh status + battery |
| `Control_LoggerFS` | control/logger_fs | LittleFS CSV data logger |
| `Control_Logger` | control/logger | Serial logger |
| `Control_GSheets` | control/gsheets | Push readings to Google Sheets |

## Debug Flags

Passed as `-D FLAG` in `platformio.ini` or `#define FLAG` before the include in Arduino IDE.

```
SYSTEM_DISCOVERY_DEBUG
SYSTEM_FRUGAL_DEBUG
SYSTEM_LITTLEFS_DEBUG
SYSTEM_MEMORY_DEBUG
SYSTEM_MQTT_DEBUG
SYSTEM_OTA_DEBUG
SYSTEM_POWER_DEBUG
SYSTEM_TIME_DEBUG
SYSTEM_WIFI_DEBUG
SYSTEM_LORAMESHER_DEBUG
CONTROL_BLINKEN_DEBUG
CONTROL_LOGGERFS_DEBUG
SENSOR_BH1750_DEBUG
SENSOR_DHT_DEBUG
SENSOR_ENSAHT_DEBUG
SENSOR_LOADCELL_DEBUG
SENSOR_MS5803_DEBUG
SENSOR_SHT_DEBUG
SENSOR_SOIL_DEBUG
```

## Example: Minimal Application (sht30)

```cpp
#include "Frugal-IoT.h"

System_Frugal frugal_iot(SYSTEM_FRUGAL_ORG, SYSTEM_FRUGAL_PROJECT, "sht30", "SHT30 Sensor");

void setup() {
  frugal_iot.configure_power(Power_Deep, 600000, 30000); // 10-min cycle, 30 s awake
  frugal_iot.pre_setup();
  frugal_iot.configure_mqtt("frugaliot.naturalinnovation.org", "dev", "public");

  frugal_iot.sensors->add(new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &I2C_WIRE, true));

  frugal_iot.setup();
}

void loop() {
  frugal_iot.loop();
}
```

## Example: Control with Wiring (climate)

```cpp
#include "Frugal-IoT.h"

System_Frugal frugal_iot(SYSTEM_FRUGAL_ORG, SYSTEM_FRUGAL_PROJECT, "climate", "Climate Control");

void setup() {
  frugal_iot.pre_setup();
  frugal_iot.configure_mqtt("frugaliot.naturalinnovation.org", "dev", "public");
  frugal_iot.configure_power(Power_Loop, 30000, 30000);

  Sensor_SHT* sht = new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &I2C_WIRE, true);
  frugal_iot.sensors->add(sht);

  frugal_iot.actuators->add(new Actuator_Digital("heating", "Heating", HEATING_PIN, "red"));
  frugal_iot.actuators->add(new Actuator_Digital("humidifier", "Humidifier", HUMIDIFIER_PIN, "blue"));

  Control_Hysteresis* ch = new Control_Hysteresis("controlheat", "Heat Control", 22.0, 1.0, 0, 100);
  frugal_iot.controls->add(ch);
  ch->inputs[0]->wireTo(sht->temperature->path());
  ch->outputs[0]->wireTo(frugal_iot.messages->setPath("heating/on"));

  frugal_iot.setup();
}

void loop() {
  frugal_iot.loop();
}
```

## LoRa / LoRaMesher

LoRa support is enabled automatically on boards that define `SYSTEM_LORAMESHER_WANT` (TTGO LoRa32, LilyGo T3-S3). Add to `platformio.ini`:

```ini
lib_deps =
    Frugal-IoT@^0.1.3
    jaimi5/LoRaMesher
    adafruit/Adafruit SSD1306@^2.5.0
    adafruit/Adafruit GFX Library@^1.10.13
build_flags =
    -D SYSTEM_LORAMESHER_FREQUENCY=915.0F  ; 868.0F Europe, 433.0F Asia
```

See `examples/loramesher/` for a gateway + node pair.

## Filesystem (LittleFS)

- Default filesystem is **LittleFS** (not SPIFFS).
- WiFi credentials: `data/wifi/<ssid>` (one file per network, content = password).
- Device config: `data/frugal_iot/` — project name, description, MQTT overrides.
- Use `board_build.filesystem = littlefs` in `platformio.ini`.

## Adding a New Sensor to an Existing Example

1. Include `Frugal-IoT.h` (already done).
2. Construct the sensor object with appropriate parameters.
3. Call `frugal_iot.sensors->add(new Sensor_Whatever(…))` **after** `pre_setup()` and **before** `setup()`.
4. Optionally wire its outputs to control inputs or actuator set-paths.
5. Enable the matching `_DEBUG` flag during development.
6. Don't forget to add the new `sensor/whatever.h` include to `Frugal-IoT.h`'s master list - a
   sensor that compiles fine on its own but was never added there won't be visible to any `.ino`.

## Testing an Example Against Local Library Changes

Every example's `platformio.ini` pulls `Frugal-IoT@^0.1.3` from the registry, not this repo's
`src/`, so building inside `examples/<name>/` only exercises the last released version - it
won't see uncommitted library changes. `lib_deps = symlink://../..` looks like the fix but
doesn't correctly resolve the library's own transitive deps (e.g. it'll fail with `fatal
error: ESPAsyncWebServer.h: No such file or directory`).

The reliable way to test an unreleased library change against a real example: temporarily
copy the example's `.ino` body into a project that references this library via PlatformIO's
local `lib/` folder auto-detection, which does resolve transitive deps correctly - e.g. the
sibling `frugal-iot-demo` project's `src/main.cpp`. If you also want the example's own
`platformio.ini` (for its board matrix/build flags) rather than just the `.ino` body, comment
out two lines in it first:
- `Frugal-IoT@^0.1.3` in `[common]` `lib_deps` - so it resolves to the local `lib/` copy
  instead of the registry
- `src_dir = .` in `[platformio]` - the host project's sources live under `src/`, not at its
  root the way a standalone example's do
Restore both files (`main.cpp` and, if swapped, `platformio.ini`) once you're done - this is a
scratch test, not a permanent change.
