# Frugal-IoT Ultrasonic

Frugal IoT - an ultrasonic distance sensor. One sketch drives any of the ultrasonic modules the
library supports, so it works for measuring a tank, a well or a river level, or for detecting that
something is there. It publishes the distance in mm, or the depth of water if you give it the
height of the sensor (see "Depth instead of distance" below).

## Which sensor - one flag

| Module | Driver | In `platformio.ini` `[common] build_flags_frugaliot` |
| --- | --- | --- |
| HC-SR04, HC-SR04P | trigger/echo | nothing - this is the default |
| AJ-SR04M, SR04M-2 (waterproof, probe on a cable) as shipped | trigger/echo | `-D SENSOR_HCSR04_MIN_MM=200` `-D SENSOR_HCSR04_MAX_MM=4500` |
| AJ-SR04M, SR04M-2 with a 120k resistor on R19 | serial | `-D SENSOR_A01NYUB_WANT` `-D SENSOR_A01NYUB_MIN_MM=200` `-D SENSOR_A01NYUB_MAX_MM=4500` |
| DFRobot A01NYUB (28cm - 7.5m) | serial | `-D SENSOR_A01NYUB_WANT` |
| DFRobot A02YYUW (3cm - 4.5m) | serial | `-D SENSOR_A01NYUB_WANT` `-D SENSOR_A01NYUB_MIN_MM=30` `-D SENSOR_A01NYUB_MAX_MM=4500` |
| RS485 / Modbus module, e.g. A01ANY4B | RS485 | `-D SENSOR_ULTRASONIC_SLAVE_ID=1` plus the two `SYSTEM_RS485_*_PIN` lines already there, commented out |

All the lines are in `platformio.ini`, commented out, under **WHICH SENSOR**. The serial and RS485
modules need an ESP32 board: an ESP8266 has no spare serial port that can receive.

## Wiring

Every module uses the **same two ESP pins**, set for each board in `platformio.ini`:

* `SENSOR_ULTRASONIC_TRIG_PIN` - from the ESP **to** the module
* `SENSOR_ULTRASONIC_ECHO_PIN` - **from** the module to the ESP

| Board (`[env:]`) | `TRIG` | `ECHO` |
| --- | --- | --- |
| `c3_pico`, `c3_oled_72x40` | GPIO3 | GPIO4 |
| `s2_mini` | GPIO16 | GPIO18 |
| `nodemcu-32s` | GPIO18 | GPIO19 |
| `d1_mini`, `d1_mini_pro` | D5 (GPIO14) | D6 (GPIO12) |
| `ttgo-lora32-v21` | GPIO13 | GPIO14 |

### The one rule: nothing above 3.3V into an ESP pin

The ESP's pins take 3.3V at most. A module powered from **5V** sends its readings back at 5V, so
the wire to `ECHO` goes through a **divider**: 1k in series from the module to the pin, and 2k from
the pin to GND (10k and 20k work as well). That gives 3.3V at the pin. A module powered from
**3V3** needs no divider, but not every module works at 3.3V. The wire from `TRIG` to the
module never needs anything: 3.3V is enough to trigger a 5V module.

### HC-SR04

| HC-SR04 | ESP |
| --- | --- |
| VCC | 5V (VBUS, or the 5V pin) - it does not work at 3.3V |
| GND | GND |
| TRIG | `TRIG` |
| ECHO | `ECHO`, **through the divider** |

The **HC-SR04P** (and other boards using the RCWL-9610A chip) works from 3.3V. Power it from 3V3 and
leave out the divider.

### AJ-SR04M and SR04M-2 - trigger/echo, as shipped

The waterproof modules with a probe on a cable. The SR04M-2 is a later version of the AJ-SR04M
board, and the header is labelled differently:

| SR04M-2 label | AJ-SR04M label | What it is | ESP |
| --- | --- | --- | --- |
| 5V | VCC | supply | 5V - see below |
| RX | Trig | trigger input | `TRIG` |
| TX | Echo | echo output | `ECHO`, through the divider if powered from 5V |
| GND | GND | ground | GND |

Leave the RST and SWIM pads alone; they are for programming the board's own chip at the factory.

**5V or 3.3V?** The AJ-SR04M is rated 3.0 - 5.5V, but SR04M-2 listings say 5V. Start at 5V with
the divider, which works either way. If you want to drop the divider, try it from 3V3. If you get
no readings, or a shorter range than the 4.5m you expected, go back to 5V.

Add `-D SENSOR_HCSR04_MIN_MM=200 -D SENSOR_HCSR04_MAX_MM=4500`. These modules use one transducer
to both send and listen, so they cannot hear anything closer than about 20 - 25cm. Closer than that
gives wrong readings, not none, which is why those readings are thrown away. If it never returns a
reading, try `-D SENSOR_HCSR04_TRIGGER_US=20`. If readings jump about, try
`-D SENSOR_HCSR04_INTERVAL_MS=100`.

#### Wiring diagram - SR04M-2 on the C3 OLED 72x40 board

```
   C3 OLED 72x40                                  SR04M-2
  ┌──────────────┐                               ┌────────────┐
  │           5V ├───────────────────────────────┤ 5V         │
  │        GPIO3 ├───────────────────────────────┤ RX  (Trig) │
  │        GPIO4 ├──────────┬────────[1k]────────┤ TX  (Echo) │
  │          GND ├────┐    [2k]          ┌───────┤ GND        │
  └──────────────┘    │     │            │       └────────────┘
                      └─────┴────────────┘
```

* The board's 5V pin is only live when the board is powered from USB or another 5V supply. On a
  board with no 5V pin, take 5V from the USB supply.
* The 1k and 2k make the divider. GPIO4 joins between them, and the 2k's other end goes to GND.
* The OLED is already on GPIO5 and GPIO6 and the LED on GPIO8, so GPIO3 and GPIO4 are free.
  Avoid GPIO2, GPIO8 and GPIO9, which decide how the C3 boots.
* The same wiring works in serial mode (next section). The RX wire is then not used, so it can
  stay connected.

### AJ-SR04M and SR04M-2 - which mode is mine in?

The mode is set by a resistor in the position marked **R19** on the board. Some batches mark it
differently, so look for an empty pair of pads near the label.

| R19 | Mode | Use |
| --- | --- | --- |
| empty (as shipped) | trigger/echo | the default build, as above |
| 120k | sends a reading over serial about every 100ms | `SENSOR_A01NYUB_WANT`, as below |
| anything else | low-power or on-request modes | not supported - remove the resistor |

To check without looking: build with `-D SENSOR_HCSR04_DEBUG` and point the probe at a wall 1 - 2m
away. If the serial monitor shows `0/5 echoes` on every reading and the wiring is right, the module
is probably in serial mode. Build again with `-D SENSOR_A01NYUB_WANT -D SENSOR_A01NYUB_DEBUG`. If
readings appear, keep that build and add the MIN_MM/MAX_MM lines from the table at the top.

### Serial modules: A01NYUB, A02YYUW, or an AJ-SR04M / SR04M-2 with 120k on R19

These modules send their reading over serial every 100 - 300ms on their TX wire. Only that one
wire goes to the ESP.

| A01NYUB / A02YYUW wire | SR04M-2 label | ESP |
| --- | --- | --- |
| Red (VCC) | 5V | 3V3 (no divider needed) or 5V (divider on the TX wire) |
| Black (GND) | GND | GND |
| White (TX) | TX | `ECHO` |
| Yellow (RX) | RX | **not connected** |

On the A01NYUB and A02YYUW the yellow wire picks the mode. Left unconnected, the module sends
filtered, steadier readings, which is what this example expects. Connected to GND, it sends raw
readings. Check the label on your cable: the wire colours are not the same on every batch.

### RS485 / Modbus modules, e.g. A01ANY4B

The module talks RS485 on two wires, A and B, which can run a long way. The ESP needs a small
**TTL-to-RS485 converter** board between them.

| Converter | Connect to |
| --- | --- |
| VCC | 3V3. If you use 5V instead, put the divider on RO |
| GND | GND (and the module's GND) |
| RO (or the pin marked as going to the MCU's RX) | `ECHO` |
| DI (or the pin marked as going to the MCU's TX) | `TRIG` |
| DE and RE, if the converter has them | join them, connect to a spare GPIO, and set `-D SYSTEM_RS485_DE_PIN=<that GPIO>` |
| A, B | the module's A and B |

Power the module itself as its label says; RS485 modules often want more than 5V. Uncomment
`SENSOR_ULTRASONIC_SLAVE_ID=1` and the two `SYSTEM_RS485_*_PIN` lines under it.

## Mounting

* Mount the probe **at least its minimum range above the closest the surface will ever come**:
  2cm for an HC-SR04, about 25cm for the AJ-SR04M / SR04M-2, 28cm for an A01NYUB. Inside that,
  readings are wrong rather than missing.
* Point it straight down, clear of the tank walls and any pipes. The beam spreads out as it goes,
  and it reports the first thing it hits.
* Only the probe is waterproof. Keep the circuit board of an AJ-SR04M / SR04M-2 in a box.

## Depth instead of distance

The sensor measures down to the surface. For the depth of water instead, set the sensor's
`offset` to the height of the probe above the tank floor in mm, and `scale` to -1. You can set
`offset` in the node's captive portal. Both can also be set over MQTT, or in the sketch, where the
commented-out lines show how. The values are saved on the node.

<!-- BEGIN generated by scripts/generate_platform_h.py - do not edit inside -->

## PlatformIO

    pio run -e <env>              # build
    pio run -e <env> -t upload    # build and flash
    pio run -e <env> -t uploadfs  # write data/ (wifi credentials, config)

`<env>` is a name from the table below, for example `c3_pico`.

Before flashing, give the node your organization's enrolment secret: copy
`templates/platformio-secrets-local.ini.template` from the library to
`platformio-secrets-local.ini` beside `platformio.ini`, and paste the line the dashboard
gives you (gear icon > Nodes > Enrolment secret) over the placeholder. Without it the node
is refused until an administrator approves it by hand. `platformio-local.ini.template` is
the optional one, for your own debug flags and serial port. Neither file is committed.

## Arduino IDE

1. **Tools > Manage Libraries**, search for **Frugal-IoT** and install it. When it asks
   about the libraries it depends on, choose **Install All**.
2. **Tools > Board > Boards Manager**, and install the one for your chip: **esp32** by Espressif Systems, or **esp8266** by ESP8266 Community (add `https://arduino.esp8266.com/stable/package_esp8266com_index.json` under **File > Preferences > Additional boards manager URLs** first).
3. **File > Examples > Frugal-IoT > hcsr04**, then **Tools > Board** and pick the row
   for your hardware.
4. **File > Save As** to keep your own copy, then give it your organization's enrolment
   secret: copy `platform-secrets-local.h.template` from the library's `templates/` folder
   (in your sketchbook, under `libraries/Frugal-IoT`) into the sketch folder (**Sketch > Show
   Sketch Folder**) as `platform-secrets-local.h`, and paste the secret from the dashboard
   (gear icon > Nodes > Enrolment secret). Without it the node is refused until an
   administrator approves it by hand. On ESP8266, put the same `#define` in the globals file instead - see below.

Opening `hcsr04.ino` from a download or a git clone does not install the library, and
the build stops with *Frugal-IoT.h: No such file or directory* - do step 1 first.

| Tools > Board | configuration | also define |
| --- | --- | --- |
| LOLIN C3 Pico | `c3_pico` |  |
| LOLIN S2 Mini | `s2_mini` |  |
| NodeMCU-32S | `nodemcu-32s` |  |
| LOLIN C3 Mini | `c3_oled_72x40` |  |
| LOLIN(WEMOS) D1 mini Pro | `d1_mini_pro` |  |
| LOLIN(WEMOS) D1 R2 & mini | `d1_mini` |  |
| TTGO LoRa32-OLED, with Board Revision = TTGO LoRa32 V2.1 (1.6.1) | `ttgo-lora32-v21` |  |

The LoRa board - TTGO LoRa32-OLED - also needs LoRaMesher, which is not in the Library Manager:

1. Download [LoRaMesher as a zip](https://github.com/loramesher/LoRaMesher/archive/refs/heads/main.zip) - leave it zipped.
2. **Sketch > Include Library > Add .ZIP Library**, and choose that file.
3. **Tools > Manage Libraries**, and install **RadioLib**, which LoRaMesher uses.

Without it the build stops with *loramesher.hpp: No such file or directory*.

Pick a board that is not in the table and the build stops with a message listing the ones
that are - rather than quietly compiling on the library's defaults, which for most boards
means the wrong pins.

## ESP8266: one extra step

The ESP8266 settings live in `esp8266/hcsr04.ino.globals.h`, and that file has to be
moved up beside `hcsr04.ino` before it is read. See `esp8266/README.md` - it is one
drag and no rename. ESP32 and PlatformIO users can ignore the folder.

## If it does not fit

*"text section exceeds available space in board"* means this board's default flash
layout has less app space than the sketch needs. **Tools > Partition Scheme** and pick
**Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)** if your board offers it. Most of
these boards do not, and then:

1. **Sketch > Show Sketch Folder**, and rename `min_spiffs.csv` to `partitions.csv`.
2. **Tools > Partition Scheme >** choose **No OTA (2MB APP/2MB SPIFFS)**.

Over-the-air updates still work despite that menu label: step 1 is what sets the real
layout and it keeps both OTA slots. The menu choice only raises the size limit the IDE
checks against.

<!-- END generated -->
