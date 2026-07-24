# Frugal-IoT Library

A platform for affordable, easily built sensor networks running on ESP32/ESP8266 microcontrollers.
Library version: 0.1.1. MIT licence. Author: Mitra Ardron.

Wiki: https://github.com/mitra42/frugal-iot/wiki
Repo: https://github.com/mitra42/frugal-iot

## Compatibility

Every example `.ino` file must work in **both**:
- **Arduino IDE** — open the `.ino` directly; dependencies installed via Library Manager
- **PlatformIO** — referenced via `lib_deps = Frugal-IoT@^0.1.1` in `platformio.ini`

Each example directory contains a `platform.h` file alongside the `.ino`. This file is
**auto-generated** from the example's `platformio.ini` by running `scripts/prerelease.bash`
(which calls `scripts/generate_platform_h.py`). It converts `-D FLAG=value` build flags into
`#define` statements and wraps board-specific defines in `#ifdef ARDUINO_BOARD_NAME` guards.

- **PlatformIO** reads flags directly from `platformio.ini`; `platform.h` is not used.
- **Arduino IDE** users include `platform.h` at the top of the `.ino` to get the same defines.

Do not hand-edit `platform.h` — regenerate it by re-running `scripts/prerelease.bash` after
changing `platformio.ini`.

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
│   ├── lcd_sht/           # HD44780 LCD showing a remote SHT node's readings over MQTT
│   ├── lilygohigrow/      # Plant watering (LilyGo HiGrow)
│   ├── ms5803/            # MS5803 pressure sensor
│   ├── power/             # Power mode demonstration
│   ├── remotedisplay/     # OLED showing a remote SHT node's readings over MQTT
│   └── sonoff/            # Sonoff relay module
└── test/
```

Each example directory contains a `.ino` file (the application) and a `platform.h` (hardware pin/address overrides).

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
| `Sensor_LoadCell` | sensor/loadcell | Weight via HX711 |
| `Sensor_DS18B20` | sensor/ds18b20 | 1-Wire temperature |
| `Sensor_MS5803` | sensor/ms5803 | Pressure + temperature |
| `Sensor_ENS160AHT21` | sensor/ens160aht21 | Air quality + temp/humidity |
| `Sensor_Button` | sensor/button | Button press events |
| `Sensor_Analog` | sensor/analog | Raw ADC |
| `Sensor_Float` | sensor/float | Arbitrary float value |
| `Sensor_UInt16` | sensor/uint16 | Arbitrary uint16 value |
| `Sensor_Health` | sensor/health | Device health metrics |
| `Sensor_GPS` | sensor/gps | GPS location (lat/lon/altitude/speed/course/hdop/satellites/UTC time) via NMEA serial module |

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

System_Frugal frugal_iot("dev", "developers", "sht30", "SHT30 Sensor");

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

System_Frugal frugal_iot("dev", "developers", "climate", "Climate Control");

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
    Frugal-IoT@^0.1.1
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

Every example's `platformio.ini` pulls `Frugal-IoT@^0.1.1` from the registry, not this repo's
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
- `Frugal-IoT@^0.1.1` in `[common]` `lib_deps` - so it resolves to the local `lib/` copy
  instead of the registry
- `src_dir = .` in `[platformio]` - the host project's sources live under `src/`, not at its
  root the way a standalone example's do
Restore both files (`main.cpp` and, if swapped, `platformio.ini`) once you're done - this is a
scratch test, not a permanent change.
