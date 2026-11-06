# Frugal-IoT Particulates

Particulate matter (PM) from a laser dust sensor, with a humidity sensor beside it. Two particulate
sensors are supported, each with its own build:

- **Nova Fitness SDS011**, the sensor in
  [sensor.community's airRohr](https://sensor.community/en/sensors/airrohr/) kit. On a serial port
  (UART). PM2.5 and PM10.
- **Seeed Grove HM3301** ("Grove - Laser PM2.5 Sensor"). On I2C. PM1.0, PM2.5 and PM10.

> **Untested as of 2026-11-06.** Both drivers (`src/sensor/sds011.h` and `src/sensor/hm3301.h`, on
> the shared `src/sensor/pm.h`) were written from the makers' documents and have not yet run against
> a real module. Reports either way are very welcome.

## What it publishes

| Topic | Unit | From |
|---|---|---|
| `sds011/pm25`, `sds011/pm10` | µg/m³ | SDS011: particles up to 2.5 µm and up to 10 µm |
| or `hm3301/pm1`, `hm3301/pm25`, `hm3301/pm10` | µg/m³ | HM3301: particles up to 1, 2.5 and 10 µm |
| `bme280/humidity`, `bme280/temperature`, `bme280/pressure` | %, °C, hPa | BME280 (the default) |
| or `sht/humidity`, `sht/temperature` | %, °C | SHT3x / SHT4x |
| or `dht/humidity`, `dht/temperature` | %, °C | DHT22 / DHT11 |

`nan` on all the PM topics means no data arrived from the sensor at all (see *Troubleshooting*).

**Why humidity:** both PM sensors count water droplets as particles, so above about 70% relative
humidity their readings are too high, sometimes far too high. Nothing here corrects for that.
Humidity is published so you can tell a polluted hour from a foggy one.

## How a reading is taken

Every 5 minutes the node wakes from deep sleep and the sensor holds it awake until it has a reading.

**SDS011:** it can be told to sleep, so each wake the node:

1. tells the SDS011 to wake (fan and laser on)
2. waits 15 s for the fan to draw fresh air through
3. averages the next 5 s of readings, dropping the highest and lowest (the same as airRohr)
4. publishes, puts the SDS011 back to sleep, and lets the node deep sleep

So each wake lasts about 20 s. The SDS011's laser is rated for about 8000 hours, which is why it
sleeps between readings.

**HM3301:** it has no sleep. Its fan and laser run whenever it has power, including while the node
sleeps. So only the first reading after power-on waits 30 s for it to settle (Seeed's figure). After
that, each wake just averages 5 s of readings, so wakes are short. The catch is that it wears out
faster than an SDS011 that sleeps.

To change the timing, set `SENSOR_SDS011_WARMUP_MS` (Nova suggest 30000), `SENSOR_HM3301_WARMUP_MS`,
or the `_READING_MS` flags in `platformio.ini`. The cycle is set in `configure_power()` in
`particulates.ino`.

For 3 minutes after power-on the node stays awake anyway, so that its WiFi access point can be
found. Readings are still taken every 5 minutes during that time.

## Choosing a build

Each board has one build ("env") per particulate sensor:

| Board | SDS011 | HM3301 |
|---|---|---|
| Seeed XIAO ESP32C3 | `xiao_esp32c3-sds011` | `xiao_esp32c3-hm3301` |
| NodeMCU-32S | `nodemcu-32s-sds011` | `nodemcu-32s-hm3301` |

The humidity sensor is chosen separately, see below. It works the same in all four.

## Wiring

### The SDS011

The SDS011 has a 7-pin connector, with each pin labelled on its circuit board. Only four pins are
used:

| SDS011 pin | Goes to | Notes |
|---|---|---|
| **5V** | the board's **5V** pin | Must be 5V. The fan does not run on 3.3V. It draws about 70 mA |
| **GND** | **GND** | |
| **TXD** | the board's SDS011 **RX** pin (table below) | Data from the sensor |
| **RXD** | the board's SDS011 **TX** pin (table below) | Commands to the sensor (wake and sleep) |
| 25um, 1um, CTL | nothing | The pulse outputs (PWM) are not used |

Cross the TX and RX lines: the sensor's **T**XD goes to the board's **R**X. The signal lines are
3.3V, so they connect directly, with no resistors.

If you are reusing an airRohr kit, its instructions number the pins from the TXD end: pin 1 TXD,
2 RXD, 3 GND, 5 5V. If your SDS011 came with a small USB adapter board, unplug it. The cable goes
straight to the ESP32.

### The HM3301

The Grove HM3301 has a 4-wire Grove cable. It goes on the same I2C pins as the humidity sensor:

| Grove wire | Goes to | Notes |
|---|---|---|
| red (VCC) | **3V3** | Seeed say 3.3V or 5V. Use 3.3V: the board's I2C pull-ups go to whatever it is supplied with, and 5V on SDA/SCL is too much for an ESP32 |
| black (GND) | **GND** | |
| white (SDA) | the board's **SDA** (table below) | |
| yellow (SCL) | the board's **SCL** (table below) | |

Take its power from the plain 3V3 pin, **not** from the switched humidity-sensor power pin. It has to
stay powered, and the switched pin cannot supply its fan.

Its I2C address is fixed at 0x40. That clashes with an INA219 or HDC1080 on the same bus, but not
with the BME280 (0x76) or SHT (0x44).

### The humidity sensor

**The default is BME280.** To use another sensor, edit `platformio.ini`:

- **SHT3x/SHT4x:** comment out `-D SENSOR_BME280_WANT` and uncomment `-D SENSOR_SHT_WANT`.
- **DHT22:** uncomment `-D SENSOR_DHT_PIN=4` in your build's `[env:]` section.

If more than one is set, the sketch uses the first of DHT, SHT, BME280.

| Sensor | Wiring |
|---|---|
| BME280 or SHT (I2C) | VCC (or VIN) to the **sensor power** pin, GND to **GND**, SDA and SCL to the board's SDA and SCL (table below) |
| DHT22 | + to the **sensor power** pin, − to **GND**, data (out) to the DHT pin (table below). A bare 4-pin DHT22 also needs a 10k resistor from data to the sensor power pin; a DHT22 on a small breakout board already has one |

The humidity sensor takes its power from a GPIO pin, not from the 3V3 pin, so the node can switch
it off between readings. It draws little enough that a pin can supply it. The pin is set by
`SENSOR_BME280_POWER3v3_PIN`, `SENSOR_SHT_POWER3v3_PIN` or `SENSOR_DHT_POWER3v3_PIN` in each
`[env:]` section. All three are set to the same pin, so the wiring is the same whichever sensor you
use. To power the sensor from 3V3 all the time instead, comment out those three lines.

With an HM3301 on the same I2C bus, the HM3301's pull-ups keep SDA and SCL at 3.3V while the node
sleeps. A switched-off BME280 or SHT can then draw a little power through those lines. It does no
harm; it only saves less.

Beware boards sold as "BME280" that actually carry a **BMP280**, which measures no humidity.
The driver checks the chip, so on that board the serial monitor shows
`bme280: wrong chip id 0x58` at startup.

### Seeed Studio XIAO ESP32C3 (`xiao_esp32c3-sds011`, `xiao_esp32c3-hm3301`)

| Signal | XIAO pad | GPIO |
|---|---|---|
| 5V for the SDS011 | **5V** | (USB 5V) |
| 3V3 for the HM3301 | **3V3** | |
| GND | **GND** | |
| SDS011 TXD to | **D7**, marked RX | 20 |
| SDS011 RXD to | **D6**, marked TX | 21 |
| I2C SDA (BME280/SHT/HM3301) | **D4** | 6 |
| I2C SCL (BME280/SHT/HM3301) | **D5** | 7 |
| DHT22 data | **D2** | 4 |
| Humidity sensor power (switched 3V3) | **D3** | 5 |

The XIAO's 5V pad carries 5V only when it is powered over USB-C. On a battery it carries the
battery voltage, which is too low for the SDS011's fan. A battery-powered SDS011 node needs a 5V
boost converter feeding the SDS011. The HM3301 runs from 3V3, so it needs none. But its fan runs
all the time, so expect a battery to last days, not months.

The XIAO has no LED you can control. The LED by the USB socket shows charging only.

### NodeMCU-32S, ESP32-WROOM-32 (`nodemcu-32s-sds011`, `nodemcu-32s-hm3301`)

| Signal | NodeMCU-32S pin | GPIO |
|---|---|---|
| 5V for the SDS011 | **5V** (some boards say **VIN**) | (USB 5V) |
| 3V3 for the HM3301 | **3V3** | |
| GND | **GND** | |
| SDS011 TXD to | **RX2** | 16 |
| SDS011 RXD to | **TX2** | 17 |
| I2C SDA (BME280/SHT/HM3301) | **21** | 21 |
| I2C SCL (BME280/SHT/HM3301) | **22** | 22 |
| DHT22 data | **4** | 4 |
| Humidity sensor power (switched 3V3) | **25** | 25 |

**Check which NodeMCU you have.** "NodeMCU" is also the name of an older **ESP8266** board (v2/v3,
"Lolin" or "Amica"), and that is the one in the airRohr kit. This example does **not** run on an
ESP8266: its only serial port that can receive is used by USB. The ESP32 board's chip is marked
ESP32 (often "ESP-WROOM-32") on its metal can. The ESP8266 board's chip is marked ESP8266 or
"ESP-12".

## Building

PlatformIO: from this folder, `pio run -e <build> -t upload` with a build name from *Choosing a
build*, e.g. `pio run -e nodemcu-32s-hm3301 -t upload`. Upload your WiFi settings as for any other
Frugal-IoT example.

Arduino IDE: choose **XIAO_ESP32C3** or **NodeMCU-32S** under Tools > Board. This builds the SDS011
version. For the HM3301, put `-DXIAO_ESP32C3_HM3301` or `-DNODEMCU_32S_HM3301` in a file called
`build_opt.h` beside the `.ino`. If the build says *"text section exceeds available space in
board"*, see the Frugal-IoT wiki on partitions.

## Troubleshooting

Uncomment `-D SENSOR_SDS011_DEBUG` or `-D SENSOR_HM3301_DEBUG` in `platformio.ini` to see each
reading on the serial monitor.

- **`no data from the SDS011` on the serial port, and `nan` on the PM topics.** No frames arrived.
  Check:
  - Does the fan spin when the node wakes? If not, the SDS011 has no 5V.
  - Is the sensor's TXD on the board's RX pin? Swapping TXD and RXD is the most common mistake.
  - Is GND connected?
- **SDS011 gives readings, but the fan never stops.** The sleep command is not reaching the
  sensor. Check the wire from the board's TX pin to the sensor's RXD.
- **`no data from the HM3301` on the serial port.** Nothing answered at 0x40. Check the four Grove
  wires, particularly that SDA and SCL are not swapped. With `-D SENSOR_HM3301_DEBUG` the serial
  monitor says `found` or `NOT FOUND` at startup.
- **Very high readings in fog, rain or a damp room.** Look at the humidity. Above ~70% this is
  expected, see *What it publishes*.
