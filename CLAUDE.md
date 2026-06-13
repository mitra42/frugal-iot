# Frugal-IoT Library

A platform for affordable, easily built sensor networks running on ESP32/ESP8266 microcontrollers.
Library version: 0.0.22. MIT licence. Author: Mitra Ardron.

Wiki: https://github.com/mitra42/frugal-iot/wiki
Repo: https://github.com/mitra42/frugal-iot

## Compatibility

Every example `.ino` file must work in **both**:
- **Arduino IDE** — open the `.ino` directly; dependencies installed via Library Manager
- **PlatformIO** — referenced via `lib_deps = Frugal-IoT@^0.0.22` in `platformio.ini`

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
│   ├── system_frugal.h/cpp   # Main controller class (System_Frugal)
│   ├── system_*.h/cpp        # WiFi, MQTT, OTA, power, FS, I2C, SPI, time, watchdog…
│   ├── sensor_*.h/cpp        # One file pair per sensor type
│   ├── actuator_*.h/cpp      # LED, digital output, OLED
│   └── control_*.h/cpp       # Logic blocks (hysteresis, logger, OLED display, Google Sheets…)
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
│   ├── gsheets/           # Google Sheets integration
│   ├── lilygohigrow/      # Plant watering (LilyGo HiGrow)
│   ├── ms5803/            # MS5803 pressure sensor
│   ├── power/             # Power mode demonstration
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

All components inherit from `System_Base` → `System_Group` → specific base class.

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

## Signal Wiring (Message Bus)

Components communicate via a path-based message bus, not direct pointers.

```cpp
// Wire a sensor output to a control input:
cc->inputs[0]->wireTo(sht->temperature->path());

// Wire a control output to an actuator's "set" topic:
cc->outputs[0]->wireTo(frugal_iot.messages->setPath("heating/on"));
```

Paths follow the pattern `<device_id>/<leaf>`. `setPath` creates a writable endpoint;
`path` creates a readable one.

## Available Sensors

| Class | File | Measures |
|-------|------|---------|
| `Sensor_SHT` | sensor_sht | Temperature + humidity (SHT30/SHT40/SHT85) |
| `Sensor_DHT` | sensor_dht | Temperature + humidity (DHT11/22) |
| `Sensor_Soil` | sensor_soil | Soil moisture (capacitive) |
| `Sensor_Battery` | sensor_battery | Battery voltage |
| `Sensor_BH1750` | sensor_bh1750 | Light (lux) |
| `Sensor_LoadCell` | sensor_loadcell | Weight via HX711 |
| `Sensor_DS18B20` | sensor_ds18b20 | 1-Wire temperature |
| `Sensor_MS5803` | sensor_ms5803 | Pressure + temperature |
| `Sensor_ENS160AHT21` | sensor_ens160aht21 | Air quality + temp/humidity |
| `Sensor_Button` | sensor_button | Button press events |
| `Sensor_Analog` | sensor_analog | Raw ADC |
| `Sensor_Float` | sensor_float | Arbitrary float value |
| `Sensor_UInt16` | sensor_uint16 | Arbitrary uint16 value |
| `Sensor_Health` | sensor_health | Device health metrics |

## Available Actuators

| Class | Notes |
|-------|-------|
| `Actuator_LEDBuiltin` | Built-in LED; added automatically on supported boards |
| `Actuator_Digital` | Any digital output (relay, LED) |
| `Actuator_OLED` | SSD1306 OLED; added automatically on supported boards |
| `Actuator_LCD` | HD44780 LCD via I2C backpack; requires `ACTUATOR_LCD_WANT` |

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

| Class | Notes |
|-------|-------|
| `Control_Hysterisis` | Single-channel on/off with deadband |
| `Control_Blinken` | LED blink pattern generator |
| `Control_OLED` | Base class for custom OLED displays |
| `Control_LoggerFS` | LittleFS CSV data logger |
| `Control_Logger` | Serial logger |
| `Control_GSheets` | Push readings to Google Sheets |

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

  Control_Hysterisis* ch = new Control_Hysterisis("controlheat", "Heat Control", 22.0, 1.0, 0, 100);
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
    Frugal-IoT@^0.0.22
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
