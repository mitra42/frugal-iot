# Frugal-IoT Library

A platform for affordable, easily built sensor networks running on ESP32/ESP8266 microcontrollers.
Library version: 0.1.6. MIT licence. Author: Mitra Ardron.

Wiki: https://github.com/mitra42/frugal-iot/wiki
Repo: https://github.com/mitra42/frugal-iot

## Compatibility

Every example `.ino` file must work in **both**:
- **Arduino IDE** — open the `.ino` directly; dependencies installed via Library Manager
- **PlatformIO** — referenced via `lib_deps = Frugal-IoT@^0.1.6` in `platformio.ini`

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
│   ├── sensor/            # One file pair per sensor type (sht, dht, soil, battery, bh1750,
│   │                       #   loadcell, ds18b20, ms5803, aht, ens160, bmx280, bme680, button,
│   │                       #   analog, float, uint16, health, gps, sensor [base class]…)
│   ├── actuator/          # LED, digital output, OLED, LCD (actuator.h is the base class)
│   └── control/           # Logic blocks (hysteresis, logger, logger_fs, blinken, carousel,
│                           #   oled, oled_loramesher, gsheets, control.h [base class]…)
├── examples/              # One subdirectory per example application
│   ├── sht30/             # Temperature + humidity with optional OLED
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

### Setting the clock, and why that moves the timers

A node can be given the time by whoever is looking at its captive portal: `System_Time` renders
the device's current time and a button that POSTs the browser's epoch and UTC offset. The AP is
always up (`System_Captive::setup()` calls `softAP()` unconditionally, not only on WiFi failure),
so this works on a site with no internet and no NTP — which is what makes time-of-day behaviour
usable there at all. It rides the ordinary message bus as `set/time/epoch` and `set/time/offset`,
so the same path works over MQTT, letting a gateway set the clock of a node that has no NTP.

**`System_Time::set()` is the only sanctioned way to step the clock**, because the wall clock and
the sleep-safe timers are the same clock:

```cpp
void timer_set(i, secs) { timers[i] = sleepSafeSecs() + secs; }   // ABSOLUTE
bool timer_expired(i)   { return timers[i] <= sleepSafeSecs(); }
uint32_t sleepSafeSecs() { gettimeofday(&tv, NULL); return tv.tv_sec; }
```

So stepping the clock moves every armed timer relative to "now":

- **Forward** (the usual 1970 → now jump on a cold boot): every armed timer expires at once and
  fires a cycle early. Harmless, and this has always happened on the first NTP sync.
- **Backward** — a phone with a wrong clock, or NTP correcting one — the timers still hold values
  from the old, higher clock and can be unreachable for **months**, silently stopping OTA,
  discovery and the watchdog's periodic work on a device nobody can reach.

Two tools on `System_Power`, used where each is possible:

| | Use when | Used by |
|---|---|---|
| `timers_shift(delta)` | The size of the step is known | `System_Time::set()` |
| `timers_clampFuture(max)` | It is not | the SNTP sync callback |

SNTP steps the clock inside the IDF and its notification callback is handed only the *new* time,
so the delta cannot be recovered there — hence the clamp, which needs no knowledge of the step.
Nothing in the library arms a timer for more than `SYSTEM_OTA_S`, so anything further out than a
day is measuring against a clock that has since moved, and is re-armed to fire now. ESP8266 has
no equivalent callback and keeps the old behaviour.

**Timezone comes from the browser as a plain UTC offset**, which `setTimezoneOffset()` turns into
a POSIX TZ string — noting that POSIX *inverts the sign*, so UTC+7 is `<+07>-7`. No DST rule is
appended and none can be: a browser reports the offset it is using now, not the zone it is in.
That is exactly right year-round where there is no DST (Indonesia and most of Asia) and an hour
out across a transition elsewhere until the button is pressed again. Encoding zones properly
would need an IANA-to-POSIX table, which is real flash on a node.

**What persists.** The offset is written to LittleFS and restored by `setup()`; the epoch
deliberately is **not** — a stored epoch would be replayed at the next boot and would set the
clock to whenever it was last written, which is worse than the RTC value it would overwrite.
`settimeofday()` moves the RTC-backed clock, so a time set once survives deep sleep and every
reset short of a power cycle.

**`System_Time` is opt-in.** `frugal_iot.time` is `nullptr` unless the sketch adds it:

```cpp
frugal_iot.system->add(frugal_iot.time = new System_Time());
```

Adding it to the group is what makes `setup()`, `dispatch()`, `captiveLines()` and
`infrequently()` run, so the button only appears on sketches that do this (`all`, `datalogger`).

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
| `Sensor` (`sensor/sensor.h`) | `std::vector<OUT*> outputs` | `Sensor_SHT` has `OUTfloat* temperature; OUTfloat* humidity;` |
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

## Invalid readings — how "there is no reading" propagates

A sensor that cannot get a reading publishes `nan` rather than publishing nothing. Before this
existed, a failed read was *silent*: `Sensor_Float::readValidateConvertSet()` dropped the value,
no message went out, and every downstream `IN` kept its last good reading indefinitely. Since
`OUT::set()` is send-on-change, "the sensor is dead" and "the value has not changed" looked
identical on the wire — so a `Control_Hysteresis` driving a valve would hold it open forever on
the last reading before the cable was cut.

**The mechanism, end to end:**

1. `validate()` fails (or the bus read fails, or the device is absent).
2. The sensor calls `setOutputsInvalid()` (`sensor.h`), or `setInvalid()` on one output where
   only some are affected.
3. `OUTfloat::setInvalid()` is `set(NAN)`, and `OUTfloat::StringValue()` serialises NaN as the
   canonical `IO_PAYLOAD_INVALID` — so `nan` goes out on the topic and to `wiredPath`.
4. `INfloat::convertAndSet()` recognises it and stores NaN; `IN::isValid()` returns false.
5. A `Control` can test `allInputsValid()` in `act()` and do something sensible.

**Three things that are easy to get wrong here:**

- **`changed()`, never `!=`.** IEEE says `NaN != NaN`, so a plain `newvalue != value` reports
  "changed" on *every* read while a sensor is invalid — republishing `nan` every cycle and
  re-running every wired control with it. `changed()` (`misc.h`) treats two NaNs as equal, so
  the transition into and out of invalid publishes once. Every `set()` and `convertAndSet()` in
  `io.cpp` uses it, including the types with no NaN, so they all read the same way.
- **Do not build the payload with `String(NAN, width)`.** Arduino's `String(double, dp)` calls
  `dtostrf(v, dp+2, dp, buf)`, which right-justifies to that width — so a width of 2 yields
  `" nan"` with a leading space while a width of 1 yields `"nan"`. The wire form would then vary
  by sensor. `StringValue()` emits `IO_PAYLOAD_INVALID` directly instead.
- **Invalid is not the same as out-of-range.** Invalid means *there is no reading*. A sensor may
  deliberately pass an extreme value outside its declared `min`/`max` straight through, and that
  value is valid — a real 70°C from a probe declared 0..50 is information, not an error. Custom
  `validate()` overrides should keep that distinction; flagging out-of-range is the UX's job
  (`frugal-iot-client` already has an `outOfRange` notion for it).

**Only the float types can express it.** There is no NaN for a `uint16` or a `bool`, and any
sentinel would be indistinguishable from a real reading. `OUT::setInvalid()` is therefore a
deliberate no-op on `OUTuint16`/`OUTbool`/`OUTtext` and `IN::isValid()` returns true for them, so
`setOutputsInvalid()` is safe to call on a sensor with mixed output types — it marks the floats
and leaves the rest. `Sensor_Uint16`, `Sensor_ENS160` and `Sensor_Health` consequently have no
invalid path at all.

**Deep sleep:** nothing here uses `millis()`, a timer slot, or RTC memory. `nan` is an ordinary
retained MQTT value, so a node waking from deep sleep or reconnecting receives the sensor's last
known state along with everything else.

**Known gap:** this detects "the sensor could not read", not "the node went away". A node that
dies while its last reading was valid leaves that value retained, and nothing currently notices.
Catching that needs a time-based staleness check on `IN`, which is a separate piece of work —
relevant once sensors and actuators live on physically separate devices.

**Other repos:** `nan` has to be understood by `frugal-iot-logger` and `frugal-iot-client`, which
each carry their own copy of `valueFromText()` (the comment in both says so). `frugal-iot-server`
needs nothing — `lib/data-loader.js` already filters `isNaN` out of the graph series.

## Available Sensors

| Class | File | Measures |
|-------|------|---------|
| `Sensor_SHT` | sensor/sht | Temperature + humidity (SHT30/SHT40/SHT85) |
| `Sensor_DHT` | sensor/dht | Temperature + humidity (DHT11/22) |
| `Sensor_AHT` | sensor/aht | Base class for the AHT20/AHT21 — not instantiated directly |
| `Sensor_AHT20` | sensor/aht | Temperature + humidity (AHT20) |
| `Sensor_AHT21` | sensor/aht | Temperature + humidity (AHT21), as on the ENS160+AHT21 breakout |
| `Sensor_Soil` | sensor/soil | Soil moisture (capacitive) |
| `Sensor_Battery` | sensor/battery | Battery voltage |
| `Sensor_BH1750` | sensor/bh1750 | Light (lux) |
| `Sensor_BMx280` | sensor/bmx280 | Base class for the BMP280/BME280 — not instantiated directly |
| `Sensor_BMP280` | sensor/bmx280 | Temperature + pressure (hPa). Freestanding, no external library |
| `Sensor_BME280` | sensor/bmx280 | Temperature + pressure (hPa) + humidity. Freestanding, no external library |
| `Sensor_BME680` | sensor/bme680 | Temperature + humidity + pressure (hPa) + gas resistance (kΩ). Also handles the BME688. Freestanding, no external library |
| `Sensor_LoadCell` | sensor/loadcell | Weight via HX711 |
| `Sensor_DS18B20` | sensor/ds18b20 | 1-Wire temperature. Bound to a probe by ROM id, not bus position - see "1-Wire" below |
| `Sensor_MS5803` | sensor/ms5803 | Pressure + temperature |
| `Sensor_ENS160` | sensor/ens160 | Air quality — AQI, TVOC, eCO2 (+ aqi500 on an ENS161). Takes temperature and humidity as **`IN`s** for its compensation |
| `Sensor_Button` | sensor/button | Button press events |
| `Sensor_Analog` | sensor/analog | Raw ADC |
| `Sensor_INA219` | sensor/ina219 | Current/voltage/power monitor — shunt (mV), bus (V), current (mA), power (mW), load (V). Freestanding, no external library |
| `Sensor_DissolvedOxygen` | sensor/dissolvedoxygen | Dissolved oxygen (mg/L) from an analog probe, temperature compensated; extends `Sensor_Analog`. Has an `IN` (as `Sensor_ENS160` does) |
| `Sensor_Float` | sensor/float | Arbitrary float value |
| `Sensor_UInt16` | sensor/uint16 | Arbitrary uint16 value |
| `Sensor_Health` | sensor/health | Device health metrics |
| `Sensor_GPS` | sensor/gps | GPS location (lat/lon/altitude/speed/course/hdop/satellites/UTC time) via NMEA serial module |
| `Sensor_Ultrasonic` | sensor/ultrasonic | Distance (mm) from an RS485/Modbus ultrasonic module (A01ANY4B); needs `SENSOR_ULTRASONIC_SLAVE_ID` |
| `Sensor_SoilModbus` | sensor/soilmodbus | Soil moisture (%) + temperature from an RS485/Modbus probe (DFRobot SEN0600 and similar); needs `SENSOR_SOILMODBUS_WANT` |

### There is no `Sensor_HT` — and `captiveLines()` is on `Sensor`

`Sensor_HT` used to be the base class for every temperature+humidity sensor (SHT, DHT, BME280,
BME680, the old ENS160+AHT21). It has been **removed**: all it did was construct the two
`OUTfloat`s, offer `set(temp, humy)`, and print those two values in the captive portal — while
blocking anything that has temperature but not humidity (a BMP280) from sharing code with
something that has both.

What replaced it:

- **The outputs live in the sensor.** Each class declares its own `OUTfloat* temperature;` and
  `OUTfloat* humidity;`, constructs them in its initialiser list and `push_back`s them onto
  `outputs`. Two lines each, and the class is then free to choose its own base — which is what
  lets `Sensor_BME280` sit under `Sensor_BMx280` next to a humidity-less `Sensor_BMP280`.
- **`set(temp, humy)` is gone**; call `temperature->set(t); humidity->set(h);`.
- **`Sensor::captiveLines()` is generic.** It prints one read-only line per entry in `outputs`
  using that output's `name` and its optional `unit`, so a sensor that just reports values needs
  no override at all — `Sensor_BME280`, `Sensor_BME680` and `Sensor_INA219` each dropped a
  hand-written one. `captiveValueLines()` returns just the `<br>Name: value unit` fragment, for
  a sensor that has something extra to show (`Sensor_ENS160` appends its two `IN`s).
- **`IO::unit`** (`system/io.h`) is a `const char*`, default `nullptr`, set on the output after
  constructing it (`temperature->unit = "C";`). Display only — it is not sent to the UX, which
  gets its labels from the server's schema — and it is not a constructor argument because that
  would mean touching every `IO` constructor in the library for a captive-portal nicety.

Sensors whose captive-portal entry is *editable* — `Sensor_Float` (calibrate), `Sensor_Soil` and
`Sensor_LoadCell` (tare) — still override `captiveLines()` with `captive->addNumber()`/
`addButton()`, and are unaffected.

### `OutputRange` — UX defaults through a shared base class

`defaults.h` is generated from the server's `modules.yaml`, keyed by **module id**:
`DEFAULT_aht21_temperature_min`, `DEFAULT_bmp280_pressure_max` and so on. A base class shared by
several chips cannot name those macros — `Sensor_AHT` does not know whether it is an `aht20` or
an `aht21` until the subclass tells it. So the subclass passes them down as an `OutputRange`
(`{min, max, color}`, in `sensor/sensor.h`):

```cpp
Sensor_AHT21::Sensor_AHT21(...)
  : Sensor_AHT("aht21", name, address, wire, retain,
      {DEFAULT_aht21_temperature_min, DEFAULT_aht21_temperature_max, DEFAULT_aht21_temperature_color},
      {DEFAULT_aht21_humidity_min, DEFAULT_aht21_humidity_max, DEFAULT_aht21_humidity_color})
  { }
```

Sensors with a fixed id (`Sensor_BME680`, `Sensor_ENS160`) just name their macros directly.
Sensors whose id comes from the sketch and whose ranges are computed at runtime — `Sensor_INA219`
scales `current`/`power` from `MAX_CURRENT`, `Sensor_Ultrasonic` takes a max — deliberately keep
those runtime values, which are more accurate than a schema constant.

**Colours come from the macros, not from literals.** `DEFAULT_<module>_<leaf>_color` is the one
place a colour is decided; writing `"blue"` or `"#0000ff"` into a constructor creates a second
place that can drift from the schema silently. Note the macro is named for the **schema module**,
which is not always the file: `Sensor_BH1750` uses `DEFAULT_lux_lux_color`, because the module is
`lux`.

Three kinds of colour literal are deliberately still there, and are not the same thing:

- **Values, not display colours.** `Actuator_Ledbuiltin`'s colour argument is what the LED
  actually glows, not how the UX draws it. A schema display colour would be the wrong source.
- **Generic defaults with no module.** `OUTtext`'s `color="#000000"` default argument belongs to
  no module or leaf, so no macro applies.
- **Leaves the schema does not describe.** `Sensor_Button` publishes `click`/`long`/`double`/
  `triple` against a schema with only `button`; `Sensor_DissolvedOxygen`'s water-temperature input
  and `Control_Gsheets` are not described at all; and `Control_Oled_LoRaMesher` defines its own
  `DEFAULT_*` macros in its header, with a comment saying it is "not in schema yet - and may never
  be". Those are schema decisions, not conversions.

**Adding a sensor means adding to `modules.yaml` too.** A module id with no entry there gets no
UX labels, and `generate-defaults.js` emits no `DEFAULT_<module>_*` macros for it. Every module
the library can publish now has one; the workflow is: edit
`frugal-iot-server/config.d/schema/{modules,topics}.yaml`, copy both to the three
`frugal-iot-logger/examples/*/config.d/schema/` directories, run
`node scripts/generate-defaults.js` in `frugal-iot-logger`, and copy the resulting `defaults.h`
over `src/defaults.h` here.

### System_I2C helpers

`System_I2C` is a plain class (not `System_Base`), one instance **per addressed device**,
holding `addr` plus a `TwoWire*` for the shared bus. Prefer these over hand-rolling:

| Method | Use |
|--------|-----|
| `initialize()` | `wire->begin(I2C_SDA, I2C_SCL)`, **de-duplicated per bus** — every device on a bus calls it from its own `setup()`. With `-D SYSTEM_I2C_DEBUG` it also runs `scan()`, once per bus |
| `sendRegister(reg, value)` | Write one byte to a register |
| `sendRegister16(reg, value)` | Write a big-endian 16-bit register |
| `send1read(cmd, bytes)` | Send a register/command byte, read N bytes back as a big-endian integer (N ≤ 4) |
| `send1read1(cmd)` | The one-byte case |
| `sendAndRead(reg, buf, len)` | Send a register byte, read `len` bytes into a buffer |
| `isPresent()` | Does anything ACK at this address? Cheap wiring check before any chip-specific id read |
| `scan()` | Print every address that ACKs **on this object's bus** |

`sendRegister`, `sendRegister16`, `send1read` and `isPresent` were added after finding the same
code hand-rolled in several sensors: the ENS160's `sendAndWait()` and the BME280's `writeReg()`
were byte-identical implementations of the register write, and `ms5803.cpp` paired
`send()`+`read()` five times where `send1read()` now does it. `initialize()` gained the per-bus
guard because every I2C sensor called `wire->begin()` — see the "unnecessary since already
called" note in the old `ens160aht21.cpp` and TODO-115/TODO-16 in `sht.cpp`. `scan()` previously scanned
the global `I2C_WIRE` regardless of which bus the object was on.

The automatic `scan()` under `SYSTEM_I2C_DEBUG` hangs off `initialize()`'s per-bus guard, so it
prints once per `TwoWire` no matter how many devices are on it. `bh1750.cpp`, `sht.cpp` and
`lcd.cpp` still call `wire->begin()` directly instead of going through `initialize()`, so they
do not trigger it - they carry their own `scan()` under their own `*_DEBUG` flags.

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

**This was the first `Sensor` with an `IN`** (`Sensor_ENS160` is the second), and there are
three traps if you add another:

1. `Sensor` has only `std::vector<OUT*> outputs` — no `inputs`. The input is a plain member.
2. `Sensor::dispatch()` wraps everything in `if (msg.module() == id)`, but a *wired* input
   receives messages published by a **different** module. So the input's `dispatch()` must be
   called **outside** that test, before delegating upward — exactly what `Control::dispatch()`
   does, and the reason it can't simply be delegated.
3. Call `input->setup()` yourself and add it to `discover()` — `Sensor` does neither for you.

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

### Sensor_BMP280 and Sensor_BME280

Both chips are driven by **one** class, `Sensor_BMx280` (`sensor/bmx280.h`), with two
three-line subclasses that fix the module id and the chip id it insists on. A BME280 is a
BMP280 plus a humidity channel, right down to the register map — same chip-id/reset/status/
ctrl_meas/config registers, same 26-byte temperature+pressure calibration block at `0x88`, same
forced-mode sequence — so the extra 7-byte humidity calibration block at `0xE1`, the `ctrl_hum`
register and two more data bytes are all the difference there is. On a BMP280 the `humidity`
output is simply `nullptr`, the same "absent output is a null pointer" pattern
`Sensor_BME680` uses for its gas channel.

Freestanding over `System_I2C` — **no external library** — in the same spirit as
`Sensor_ms5803`. The compensation arithmetic and calibration unpacking are ported from Bosch's
own reference driver, `boschsensortec/BME280_SensorAPI` (`bme280.c`, `BME280_DOUBLE_ENABLE`),
which is BSD-3-Clause and therefore compatible with this library's MIT licence — the
attribution notice at the top of `bmx280.h`/`.cpp` is the requirement. Port rather than
reimplement, because `dig_h4`/`dig_h5` sign-extend the MSB *before* shifting
(`(int16_t)(int8_t)reg_data[3] * 16`), which is easy to get subtly wrong and yields
plausible-but-incorrect humidity.

`double` not `float`: the pressure polynomial divides by constants up to `2147483648.0`,
beyond a 24-bit float mantissa. It runs once per read cycle, so the cost is irrelevant.

**The chip id is enforced, not sniffed.** `setup()` resets the device and requires `0x60` for a
`Sensor_BME280`, or `0x58` (plus Bosch's `0x56`/`0x57` engineering samples) for a
`Sensor_BMP280`, calling `setupFailed()` otherwise. The two boards look identical and are sold
interchangeably, and a BME280 driven as a BMP280 would publish perfectly plausible temperature
and pressure while silently having no humidity — so the sketch has to say which one is fitted.
`0x61` is a BME680/BME688, a different register map entirely.

Reads in Bosch's **weather monitoring** configuration — forced mode, 1× oversampling on all
channels, IIR filter off — so the chip sleeps between reads, which suits one reading per wake
cycle. Each read also rejects an all-`0xFF` data block, because Bosch's compensation clamps to
the rated range and would otherwise silently publish 85 °C for a disconnected device.

Altitude is deliberately **not** published — it is a re-expression of pressure against an
assumed sea-level reference, better derived downstream from `<id>/pressure`.

```ini
; platformio.ini — the classes are always compiled; these flags are main.cpp's per-board switch
build_flags =
    -D SENSOR_BME280_WANT
    ;-D SENSOR_BME280_ADDRESS=0x77   ; default 0x76; 0x77 if SDO is tied high
    ;-D SENSOR_BMP280_ADDRESS=0x77   ; ditto for the BMP280
    ;-D SENSOR_BME280_DEBUG          ; either DEBUG flag turns on the shared debug output
    ;-D SENSOR_BMP280_DEBUG
```

```cpp
frugal_iot.sensors->add(new Sensor_BME280("BME280"));   // bme280/temperature|humidity|pressure
frugal_iot.sensors->add(new Sensor_BMP280("BMP280"));   // bmp280/temperature|pressure
// or: new Sensor_BME280("BME280", 0x77, &I2C_WIRE, true)
```

Verification: the port was cross-checked against Bosch's functions on the host over 200,000
randomized calibration/raw-value combinations — calibration unpacking, 20/16-bit raw
assembly, `t_fine`, and all three compensated outputs were bit-identical. That check predates
the split into `bmx280.cpp`, which moved the arithmetic unchanged.

### Sensor_AHT20 and Sensor_AHT21

One base class (`Sensor_AHT`, `sensor/aht.h`) and two subclasses that differ **only** in their
module id — `aht20/temperature` vs `aht21/temperature`. As far as this driver is concerned the
two chips are the same device: same address, same soft-reset/trigger/status commands, same
20-bit humidity-then-temperature layout, same conversion. The subclasses exist so the topics,
and the sketch, say which chip is actually fitted.

Freestanding over `System_I2C`; lessons and some bits from `adafruit/Adafruit_AHTX0` (MIT).
This is the AHT half of the old `sensor/ens160aht21.cpp` — see `Sensor_ENS160` below.

`SENSOR_AHT_CMD_INIT` defaults to `0xE1`, which is Adafruit's value and what the ENS160+AHT21
boards here have always been sent. Both datasheets specify `0xBE` (`0xE1` is the AHT10's), and
these parts are factory calibrated either way, so it is close to a no-op — but it is a `#define`
so a board that objects can be given `0xBE` without touching the driver.

Two things the old code got wrong, fixed here: the busy-wait had **no timeout**, so a missing
chip spun forever (a watchdog reset, not an error message); and nothing validated the reading,
so the characteristic 0 °C/0 % failure was published as real data.

```cpp
frugal_iot.sensors->add(new Sensor_AHT21("AHT21"));
// ;-D SENSOR_AHT_ADDRESS=0x39  ;-D SENSOR_AHT_CMD_INIT=0xBE  ;-D SENSOR_AHT_DEBUG
```

### Sensor_ENS160 — and why the ENS160+AHT21 is now two sensors

`sensor/ens160.h`. Publishes `aqi` (1–5), `tvoc` (ppb) and `eco2` (ppm), plus `aqi500` on an
ENS161. Was half of `sensor/ens160aht21.cpp`, which drove both chips of the common
"ENS160+AHT21" breakout from a single class.

**The split.** The ENS160 needs an ambient temperature and humidity to compensate its gas
plate, but it does not care where they come from — the AHT21 sharing the breakout, an SHT30
elsewhere on the node, or a reading published by another node entirely. Welding the two chips
together made the AHT21 unusable on its own, made the ENS160 unusable without one, and hid the
dependency from the UX. So the compensation values are now `IN`s (the `Sensor_DissolvedOxygen`
shape) that default to `aht21/temperature` and `aht21/humidity` — the combined board still
works with no wiring in the sketch, and anything else is one `wireTo()` away.

```cpp
Sensor_AHT21* aht = new Sensor_AHT21("AHT21");
Sensor_ENS160* ens = new Sensor_ENS160("ENS160");
frugal_iot.sensors->add(aht);
frugal_iot.sensors->add(ens);
ens->temperature->wireTo(aht->temperature->path()); // What setup() would do anyway
ens->humidity->wireTo(aht->humidity->path());
// ;-D SENSOR_ENS160_ADDRESS=0x52  ;-D SENSOR_ENS160_TEMPERATURE_PATH=\"sht/temperature\"
// ;-D SENSOR_ENS160_HUMIDITY_PATH=\"sht/humidity\"  ;-D SENSOR_ENS160_DEBUG
```

`eco2` is *equivalent* CO2 — derived from the VOC reading, not measured, so it will not see
CO2 from breathing unless VOCs come with it.

Three fixes came with the split: the part id was previously read only inside an `#ifdef DEBUG`,
leaving `isENS161` **uninitialised** in a normal build (and with it, whether `aqi500` meant
anything); `aqi500` is now dropped from `outputs` in `setup()` on an ENS160 rather than
discovered as a topic that only ever carries its initial 0; and the wait for new data has a
timeout instead of spinning forever on a missing chip.

### Sensor_BME680

Same shape as `Sensor_BME280` — `temperature` and `humidity` outputs, freestanding over
`System_I2C`, no external library — plus a `pressure` output in hPa and a `gas` output in kΩ.
It does **not** share `Sensor_BMx280`: the BME680's register map, calibration layout and
compensation are a different Bosch driver altogether, so there would be nothing to share but
the name.

```ini
; platformio.ini — the class is always compiled; this flag is main.cpp's per-board switch
build_flags =
    -D SENSOR_BME680_WANT
    ;-D SENSOR_BME680_ADDRESS=0x77       ; default 0x76; 0x77 if SDO is tied high
    ;-D SENSOR_BME680_HEATER_TEMP_C=320  ; gas plate target, capped at 400 by the chip
    ;-D SENSOR_BME680_HEATER_MS=150      ; hold time before the plate is sampled
    ;-D SENSOR_BME680_DEBUG
```

```cpp
frugal_iot.sensors->add(new Sensor_BME680("BME680"));
// Temperature/humidity/pressure only - no heater, no gas output, no 150ms wait:
// new Sensor_BME680("BME680", SENSOR_BME680_ADDRESS, &I2C_WIRE, true, false)
```

**Ported from a different upstream to bmx280.cpp.** Bosch retired `BME680_driver`; the
current reference is `boschsensortec/BME68x-Sensor-API` (`bme68x.c`), also BSD-3-Clause,
covering both chips. And unlike bmx280.cpp this port is **`float`, not `double`** — Bosch's
default variant for this chip is single precision (`BME68X_USE_FPU`), and matching it exactly
is what makes a bit-for-bit host check possible. There is no RobTillaart BME680 library to
cross-read against; the ones that exist (Adafruit, Zanduino, DFRobot) all wrap Bosch's driver.

**BME680 and BME688 are both accepted.** They share chip id `0x61` and are told apart by the
variant id at register `0xF0` (`0x00` = BME680, `0x01` = BME688). The variant selects a
*completely different* gas-resistance formula and a different `run_gas` bit pattern, and
getting it wrong yields a plausible-but-wrong resistance rather than an obvious failure — so
`setup()` reads it and every read honours it.

**Gas resistance is not an air quality index.** It is the resistance of a heated metal-oxide
plate: it falls as reducing (VOC) gases rise, so higher is cleaner air, but the absolute value
drifts with humidity, temperature and age. Turning it into IAQ requires Bosch's BSEC, a
closed-source per-architecture binary that cannot be redistributed under this licence, so it
is deliberately not attempted — publish the trend and derive an index downstream. The reading
is skipped (rather than published wrong) unless the chip's `gas_valid` **and** `heat_stab`
status bits are both set; the first read after power-up typically has neither.

**Cost of the gas channel.** The heater draws order 12 mA for `HEATER_MS`, and the read
*blocks* for that long too (~12 ms of TPH conversion plus the heater time, against the BME280's
~8 ms). Pass `wantGas=false` on a battery node: the heater is left off, the wait disappears,
and the `gas` output is never created rather than publishing a value that never changes.

Two other differences from `Sensor_BME280` worth knowing:

- **All measurement registers are rewritten before every read**, not once in `setup()`. A chip
  that has been power-cycled between reads (`Sensor::prepare()`/`recover()`) comes back with
  the heater off and 0× oversampling, which would keep publishing temperature and humidity
  quite happily — the failure would be invisible.
- **The heater setpoint depends on ambient temperature**, so `calc_res_heat` is recomputed each
  cycle from the last good reading instead of against Bosch's assumed 25 °C.

Bosch's BME680 compensation, unlike the BME280's, does *not* clamp to the rated range, so
`validate()` is what keeps nonsense off MQTT; an all-`0xFF` field read (absent device) is
rejected separately, as is a conversion that never sets the new-data bit.

Verification: same treatment as the BME280 — the port was cross-checked on the host against
Bosch's own `calc_*` functions (compiled from `bme68x.c`) over 200,000 randomized
calibration/raw combinations plus 20,000 randomized register maps for the calibration
unpacking. Temperature, pressure, humidity, `t_fine`, both gas formulas, `res_heat` and
`gas_wait` were bit-identical, and all 26 unpacked coefficients matched.

### 1-Wire (`system/onewire.h`) and how DS18B20 probes are bound

`System_OneWire` is one object per physical bus, shared by every device on it - the same split as
`System_RS485`/`System_Modbus` and `System_I2C`/`TwoWire`. Two things drove it:

**Position is not identity.** `DallasTemperature::getTempCByIndex(n)` re-walks the OneWire search
tree on every read and returns whatever sits at position *n*. Add, remove or replace a probe and
everything after it renumbers — so two believable temperatures end up attributed to the wrong
things, with nothing reporting an error. `Sensor_DS18B20` addresses by ROM id instead.

**A conversion is expensive and shared.** `requestTemperatures()` broadcasts a convert to the
whole bus and then blocks until it completes — 750 ms at 12-bit, because `waitForConversion`
defaults to true. A sensor owning its own bus object pays that itself, so three probes on one pin
cost 2.25 s of blocking per cycle for one conversion's worth of information. The bus converts at
most once per `SYSTEM_ONEWIRE_RECONVERT_MS` (1 s), so the first sensor in a `periodically()` pass
pays the 750 ms and the rest read the scratchpad it filled. `converted` starts false and the
object is rebuilt by the restart that deep sleep really is, so the first read after any boot or
wake always converts rather than reading a scratchpad nothing ever filled. `millis()` is the right
clock here — this is sub-cycle timing, and the flag covers the sleep case.

**Binding needs no attention in the ordinary case, and is never typed into a sketch.** A stored
binding whose probe is present is always used. Beyond that there is exactly one automatic rule:

> If exactly one sensor on the bus is unbound **and** exactly one probe is unclaimed, they are
> matched up.

That single rule covers everything worth automating:

| Situation | What happens |
|---|---|
| One sensor, one probe, nothing configured | Matched — the ordinary node needs no configuration at all |
| A probe replaced on a multi-probe bus | The others keep their stored bindings, so the orphaned sensor and the new probe are the only two left over, and are matched |
| Binding a multi-probe bus by hand | Name all but one; the last follows |
| Two or more unbound, or two or more unclaimed | Left alone — guessing which probe is the air one and which is the battery one is precisely the silent mis-attribution that ROM-id addressing exists to prevent. Unbound sensors publish `nan` and the portal lists the ids to choose from |
| A stored binding whose probe has gone | Dropped, so the sensor becomes an orphan and the rule above may re-match it. The stored id stays on disk on purpose: if that probe is reconnected, the explicit choice wins again |

Binding by hand is the captive portal or MQTT, **not** the frugal-iot-client dashboard, and that
is a decision rather than an oversight. The client cannot enumerate a 1-Wire bus — it only sees
MQTT — so offering a list of discovered probes there would mean the node publishing its bus
contents purely for a remote client to re-display, plus a new schema key and a new widget type,
across four repos. It would buy very little: the automatic rule above covers a single-probe node
and a replaced probe, so the only moment a human is needed is commissioning a bus with two or
more probes on it — which is exactly when someone is stood next to the hardware and the node's
own AP is the easiest thing to reach.

An automatic match is deliberately **not** persisted — storing it would mean that replacing the
probe left the node bound to an id that no longer exists, turning a setup that works into one
that does not, for no gain, since the same match is made again on the next boot.

The matching is the one decision no single sensor has the information to make, so it lives on the
bus (`System_OneWire::resolveUnbound()`, reached through a small `OneWireDevice` interface so that
`system/` need not know about `sensor/`). It runs on the first read rather than in `setup()`,
because at setup time the other sensors on the bus may not have read their own config yet;
`periodically()` only runs once the whole group is set up. It re-runs after any binding changes,
which is what lets naming the second of three probes pull in the third.

Binding is `set/<sensorid>/id = <romid>`, persisted to LittleFS, so the captive portal, the UX and
MQTT all reach it the same way. The portal shows the dropdown only when there is more than one
probe — with one there is nothing to choose and a row of hex is noise. So a multi-probe node is
given meaningful sensor ids in the sketch and bound once, on site, from a phone:

```cpp
System_OneWire* ow = System_OneWire::forPin(SENSOR_DS18B20_PIN);
frugal_iot.sensors->add(new Sensor_DS18B20("ds18b20-air",  "Air Temperature",     ow, true));
frugal_iot.sensors->add(new Sensor_DS18B20("ds18b20-batt", "Battery Temperature", ow, true));
```

The single-probe case stays one line and needs no bus object — the pin-taking constructor calls
`System_OneWire::forPin()`, which returns the shared bus for that pin, so two sensors on one pin
share automatically whether or not the sketch knows buses exist.

```cpp
frugal_iot.sensors->add(new Sensor_DS18B20("ds18b20", "Soil Temperature", SENSOR_DS18B20_PIN, true));
```

**Setup cost, which every deep-sleep wake pays.** `begin()` is ~90 ms — a 50 ms settle plus an
enumeration, retried up to three times only if nothing is found — and `getDeviceCount()` is free,
returning the count `begin()` cached. Sharing the bus means that is paid once rather than per
sensor, and the old per-sensor dummy `requestTemperatures()` in `setup()` is gone, so a
three-probe bus went from roughly 2.5 s of every wake to about 110 ms. (That dummy conversion
carried a comment saying it was needed "to reset OneWire which seems to fail otherwise"; it looks
redundant given the first real read converts anyway, but it was clearly added empirically and has
not been re-tested on hardware.)

**`validate()` does not reject 0.0 °C.** It used to, presumably to catch a startup artifact. That
was survivable while a rejected reading was silently dropped, but once invalid readings began
being published it meant a probe at freezing reported "no reading" and any wired
`Control_Hysteresis` held. The disconnected sentinel (`DEVICE_DISCONNECTED_C`, -127 — the same
value OSPIT uses) and the 85 °C power-on value are still rejected.

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

### Sensor_SoilModbus

Soil moisture and temperature from an RS485/Modbus probe, one instance per probe. Ported from
OSPIT's `modbr.lua`, which is the authority on the register layout: function `0x03`, two
consecutive holding registers from `0x0000`, moisture first then temperature, **both scaled by
ten, and temperature signed two's complement**. Without that sign handling a probe below freezing
reads as about +6500 °C. The decode is checked against the Lua on the host, including the
boundary at 0xFFFF (-0.1 °C) and both ends of the datasheet range.

```cpp
System_RS485* rs485 = new System_RS485(&Serial2);   // one transceiver, shared
frugal_iot.sensors->add(new Sensor_SoilModbus("soil1", "Sector 1", 1, rs485, true));
frugal_iot.sensors->add(new Sensor_SoilModbus("soil2", "Sector 2", 2, rs485, true));
```

**Slave ids are constructor arguments, not a build flag.** `Sensor_Ultrasonic` is enabled by
`SENSOR_ULTRASONIC_SLAVE_ID` because a node has one of those; an irrigation node has one probe per
sector on the same multi-drop bus, so `SENSOR_SOILMODBUS_WANT` turns the class on and each
instance carries its own id.

**A probe that does not answer publishes `nan` on both outputs** rather than leaving stale
readings standing - see "Invalid readings" above. That is the same signal OSPIT carries as `-127`,
and it is what lets an irrigation control skip a sector whose probe is missing by asking
`isValid()` instead of comparing against a magic number. The bus's retry backoff means an absent
probe costs one 2 s stall per `SYSTEM_MODBUS_RETRY_CYCLES` cycles rather than one every cycle.

**`System_Modbus::readRegisters(reg, count, out)`** was added for this: `readRegister` reads one,
and reading moisture and temperature as two transactions would double the bus time and could pair
a value from one moment with a value from another.

`SENSOR_SOILMODBUS_REGISTER` (default `0x0000`) moves the pair, since DFRobot's soil range varies
by part - some add conductivity and pH.

## Available Actuators

| Class | File | Notes |
|-------|------|-------|
| `Actuator_LEDBuiltin` | actuator/ledbuiltin | Built-in LED; added automatically on supported boards |
| `Actuator_Digital` | actuator/digital | Any digital output (relay, LED) |
| `Actuator_OLED` | actuator/oled | SSD1306 or SSD1327 OLED; added automatically on supported boards. See "Two OLED chips" below |
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

### Two OLED chips, chosen at compile time

`Actuator_OLED` drives either an SSD1306 (1 bit per pixel, usually I2C, what every board with a
built-in display carries) or an SSD1327 (4-bit greyscale, 128x128, usually SPI, always externally
wired). Which one is a **compile-time** choice: a board has exactly one display, so there is no
reason to pay a vtable and an indirection per call on a device where the redraw is already the
expensive part.

**What a control must do to work on both.** Two rules, and neither is enforced by the compiler —
code that breaks them builds cleanly and then draws nothing:

```cpp
auto* display = &frugal_iot.oled->display; // not Adafruit_SSD1306*, which is only one of the two
display->setTextColor(OLED_FG);            // not SSD1306_WHITE
```

`OLED_FG`/`OLED_BG` exist because the chips disagree about white: it is `1` on an SSD1306 and
`0xF` on an SSD1327. A control written with `SSD1306_WHITE` compiles perfectly against an
SSD1327 and draws in the darkest grey there is, i.e. invisibly.

**There are two offset wrappers, not one template**, and that is deliberate. The wrapper exists so
(0,0) means the first visible pixel on panels with a dead margin. Which methods need offsetting
depends on what the driver overrides: `Adafruit_SSD1306` has its own fast-path `drawFastHLine`,
`drawFastVLine` and `fillRect` that bypass `drawPixel`, so all four need it — whereas
`Adafruit_SSD1327` goes through `Adafruit_GrayOLED`, which overrides nothing but `drawPixel`, so
the other three fall through to `Adafruit_GFX`'s generic versions that themselves call
`drawPixel`. Offsetting those in the wrapper as well would apply the offset twice.

**Configuring an externally wired panel.** No board has an SSD1327 built in, so there is nothing
to key a board `#elif` on - chip, interface and pins all come from build flags in a board env:

```ini
-D ACTUATOR_OLED_WANT
-D ACTUATOR_OLED_IS_SSD1327
-D ACTUATOR_OLED_SPI_SCLK=18 -D ACTUATOR_OLED_SPI_MOSI=23
-D ACTUATOR_OLED_SPI_CS=5 -D ACTUATOR_OLED_SPI_DC=16 -D ACTUATOR_OLED_SPI_RST=17
```

Omitting `ACTUATOR_OLED_SPI_CS` selects the I2C form instead, and size defaults to the SSD1327's
native 128x128.

**There is deliberately no default chip.** Every board either names one in its `#elif` in
`oled.h` or has one in its build flags; anything else stops at `#error have not defined OLED
chip driver`. For the same reason every place the two chips diverge - the driver include, the
colours, the offset wrapper, the constructor and the bring-up in `setup()` - lists the chips it
supports explicitly and `#error`s on anything else, rather than falling through to an `#else`
that assumes an SSD1306. Someone adding a display to a board that has none built in must say
which chip it is: guessing wrong compiles cleanly and then draws nothing, which is the most
expensive kind of wrong on a device you have to walk to.

### A board is allowed to have no built-in LED

`actuator/ledbuiltin.h` used to `#error` unless `LED_BUILTIN` was defined, and it is included
unconditionally, so *every* board had to name an LED pin - even though `System_Frugal` only adds
the actuator inside `#ifdef LED_BUILTIN` and nothing else refers to it. The requirement bought
nothing, and the only way past it was to name a pin that does not exist; on a custom board that
pin is likely to be doing something else, and on the FF-ESP32-OpenMPPT the obvious guess, GPIO 2,
is its 1-Wire bus.

The header and its `.cpp` now compile to nothing when `LED_BUILTIN` is undefined. **No flag is
needed and none should be added** - the absence of `LED_BUILTIN` is the whole signal.
`system/ota.cpp` needed the same `#ifdef`, having passed `LED_BUILTIN` to `setLedPin()`
unconditionally.

### FF-ESP32-OpenMPPT: the display and the soil probes are mutually exclusive

`examples/all` has an `ff_openmppt_ssd1327` env for the board OSPIT runs on. Worth knowing before
planning anything for it: **the SPI panel and the Modbus soil probes cannot both be fitted.** The
panel needs dc=16 and rst=17, which are the UEXT header's RX_2/TX_2 - and those are exactly the
pins OSPIT drives UART2 on for its RS485 probes (`uart.setup(2, ..., {tx = 17, rx = 16})`). That
is why OSPIT's own `init.lua` loads `SSD1306.lua` and leaves the SSD1327 line commented out: on an
irrigation controller the probes win. The I2C SSD1306 on pins 21/22 has no such conflict.

There is only **one** FF env, also deliberately. Arduino has no concept of environments - it
compiles one configuration per board - so `generate_platform_h.py` keeps the first env targeting a
given `ARDUINO_*` macro and marks any later one `DISABLED`. Two envs on one board therefore look
fine in `platformio.ini` and silently build the same firmware twice, which is how a first attempt
at this "compiled the SSD1327 env" three times without ever compiling an SSD1327. If you add a
second env for a board that already has one, check the generated `platform.h` for `DISABLED`
before believing a green build. (`lilygo_t3_s3_sx127x_sht`, `heltec_wifi_lora_32_V32` and
`tbeam_oled` are all in this state today.)

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
SENSOR_AHT_DEBUG
SENSOR_BH1750_DEBUG
SENSOR_BME280_DEBUG / SENSOR_BMP280_DEBUG (either sets SENSOR_BMX280_DEBUG)
SENSOR_BME680_DEBUG
SENSOR_DHT_DEBUG
SENSOR_ENS160_DEBUG
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

## Example: Control with Wiring

```cpp
#include "Frugal-IoT.h"

System_Frugal frugal_iot(SYSTEM_FRUGAL_ORG, SYSTEM_FRUGAL_PROJECT, "climate", "Climate Control");

void setup() {
  frugal_iot.configure_power(Power_Loop, 30000, 30000);
  frugal_iot.pre_setup();
  frugal_iot.configure_mqtt("frugaliot.naturalinnovation.org", "dev", "public");

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
    Frugal-IoT@^0.1.6
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

## Flash size

`FLASH_SIZE.md` is the user-facing page (wiki-ready) on what to do when a build no longer fits,
ordered by payoff — partition table, `-fno-exceptions`, sensor costs, debug flags, languages.

Three scripts in `scripts/` support it. They find the project by walking up from the working
directory for a `platformio.ini`, so they run from any project that has this library, not just
from here:

| Script | Does |
|--------|------|
| `size_report.py <env>` | Per-library and per-source-file flash from the linker map, plus which objects were discarded entirely. Needs `-Wl,-Map=$BUILD_DIR/firmware.map` in `build_flags` |
| `size_check.py` | Builds the envs listed in the project's `size_baseline.json` and fails past a 2% growth margin. `--update --note "why"` re-records |
| `price_modules.py -e <env>` | What each optional sensor/actuator/control costs, by building a bare sketch then one per module. Never touches `src/main.cpp` |

Two things worth knowing before trusting any size number:

- **PlatformIO's `Flash: nn%` under-reports on RISC-V** by the size of `.eh_frame` (~64 KB on a
  C3). Use `ls -l .pio/build/<env>/firmware.bin`. Xtensa folds `.eh_frame` into `.flash.rodata`,
  so its figure is about right.
- **`size_report.py` reconciles itself against the ELF section headers** and says so if the
  numbers do not add up. That check exists because the linker map is written for humans and is
  easy to misread — merged string pools in particular are credited to whichever input section
  happened to be placed first, which once attributed 137,557 bytes to a 1-byte section.

On ESP32 the pioarduino platform sets `lib_archive = False` ("to make weak defs in framework and
libs possible"), so **every** `.cpp` of every library is linked as a loose object rather than
pulled from an archive on demand. Unused code is kept out purely by `-ffunction-sections
-fdata-sections` plus `--gc-sections`. That works — 27 of 58 objects and every unused third-party
sensor library contribute zero bytes — but it is more fragile than archive laziness: anything
anchored by a **file-scope constructor** survives along with everything it references. That is how
LoRaMesher's `std::ostringstream` was pulling in 230 KB of `std::locale`.

## Adding a New Sensor to an Existing Example

1. Include `Frugal-IoT.h` (already done).
2. Construct the sensor object with appropriate parameters.
3. Call `frugal_iot.sensors->add(new Sensor_Whatever(…))` **after** `pre_setup()` and **before** `setup()`.
4. Optionally wire its outputs to control inputs or actuator set-paths.
5. Enable the matching `_DEBUG` flag during development.
6. Don't forget to add the new `sensor/whatever.h` include to `Frugal-IoT.h`'s master list - a
   sensor that compiles fine on its own but was never added there won't be visible to any `.ino`.

## Testing an Example Against Local Library Changes

Every example's `platformio.ini` pulls `Frugal-IoT@^0.1.6` from the registry, not this repo's
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
- `Frugal-IoT@^0.1.6` in `[common]` `lib_deps` - so it resolves to the local `lib/` copy
  instead of the registry
- `src_dir = .` in `[platformio]` - the host project's sources live under `src/`, not at its
  root the way a standalone example's do
Restore both files (`main.cpp` and, if swapped, `platformio.ini`) once you're done - this is a
scratch test, not a permanent change.
