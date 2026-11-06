// Deep Sleep issues: as Sensor_PM (sensor/pm.h) - the node is held awake for the warm-up and reading, about 20s a cycle. The module is told to sleep by command and stays asleep across the ESP's deep sleep (its 5V is not switched), so each wake starts with a warm-up.
/*
 * Nova Fitness SDS011 laser particulate sensor - PM2.5 and PM10 over a UART.
 *
 * NOTE - AS OF 2026-11-05 THIS IS UNTESTED CODE - written from Nova's datasheet and protocol
 * document, not yet run against a module. Bug reports or confirmation that it works very welcome.
 *
 * This is the sensor in sensor.community's "airRohr" kit. Their firmware
 * (github.com/opendata-stuttgart/sensors-software) is GPL-3, so none of it is used here - this
 * is written from Nova's "Laser Dust Sensor Control Protocol V1.3", copying only the timing
 * (warm up 15s, then average 5s of readings dropping the highest and lowest), so readings
 * should be comparable with theirs.
 *
 * Freestanding - no external library.
 *
 * Protocol (9600 baud 8N1):
 *   Data, sent by the module once a second while it is awake ("active reporting" mode):
 *     AA C0 PM25_L PM25_H PM10_L PM10_H ID1 ID2 CHK AB       10 bytes
 *     PM2.5 = (PM25_H*256 + PM25_L) / 10 ug/m3, likewise PM10.  CHK = sum of the 6 bytes after C0
 *   Commands, sent to the module:
 *     AA B4 CMD D1 D2 0 0 0 0 0 0 0 0 0 0 FF FF CHK AB        19 bytes, CHK = sum of CMD..FF FF
 *     06 01 00 - sleep (fan and laser off)       06 01 01 - wake
 *     02 01 00 - active reporting mode           08 01 00 - continuous (no self duty-cycle)
 *   The module answers commands with an AA C5 ... frame, which the parser here simply ignores.
 *
 * How a reading is taken: Sensor_PM's cycle - see sensor/pm.h. This class only supplies the
 * protocol: wake and sleep commands, and frames from the UART.
 *
 * Module:
 *   Supply  5V, ~70mA running, a few mA asleep. MUST be 5V - the fan will not run on 3V3.
 *   Logic   3.3V TTL, so the TX/RX lines go straight to the ESP, no divider.
 *   Laser   rated ~8000 hours, which is why it is slept between readings.
 *   Range   0 - 999.9 ug/m3
 *
 * UART: pass the HardwareSerial and the two ESP pins. ESP32 only - ESP8266 has no free UART that
 * can receive. ESP32-C3/S2 have no Serial2, so use Serial1 there (examples/particulates does on all boards).
 *
 * Output published to MQTT:
 *   sds011/pm25   - PM2.5 in ug/m3
 *   sds011/pm10   - PM10 in ug/m3
 *
 * Build flags:
 *   (the library reads no pin flag - the pins are constructor arguments; examples/particulates passes
 *    SENSOR_SDS011_RX_PIN and SENSOR_SDS011_TX_PIN)
 *   SENSOR_SDS011_WARMUP_MS (15000)  - fan running before readings count. Nova suggest 30s
 *   SENSOR_SDS011_READING_MS (5000)  - readings averaged, at one a second
 *   SENSOR_SDS011_DEBUG              - Serial debug output
 */

#ifndef SENSOR_SDS011_H
#define SENSOR_SDS011_H

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "sensor/pm.h"

#ifndef SENSOR_SDS011_WARMUP_MS
  #define SENSOR_SDS011_WARMUP_MS 15000 // As airRohr; Nova suggest 30000
#endif
#ifndef SENSOR_SDS011_READING_MS
  #define SENSOR_SDS011_READING_MS 5000 // As airRohr - five frames
#endif

// TODO - remove once defaults.h is regenerated from modules.yaml, which now has sds011. Until then
// these stand in, and then defaults.h (included via _settings.h) takes over.
#ifndef DEFAULT_sds011_pm25_min
  #define DEFAULT_sds011_pm25_min 0
  #define DEFAULT_sds011_pm25_max 100
  #define DEFAULT_sds011_pm25_color "#a0522d"
#endif
#ifndef DEFAULT_sds011_pm10_min
  #define DEFAULT_sds011_pm10_min 0
  #define DEFAULT_sds011_pm10_max 200
  #define DEFAULT_sds011_pm10_color "#696969"
#endif

#ifdef ESP32 // Compiles to nothing elsewhere - see the UART note above
class Sensor_SDS011 : public Sensor_PM {
  public:
    Sensor_SDS011(const char * const name, HardwareSerial* serial, uint8_t rx_pin, uint8_t tx_pin,
      bool retain = true);
  protected:
    HardwareSerial* serial;
    const uint8_t rx_pin;
    const uint8_t tx_pin;
    uint8_t frame[10];         // Sliding window - frame[0] is the oldest byte
    uint8_t frameLen = 0;

    void setup() override;
    void command(uint8_t cmd, uint8_t d1, uint8_t d2);
    void wakeModule() override;
    bool sleepModule() override;
    bool readSample(uint16_t* v) override;
    void reportNoData() override;
};
#endif // ESP32

#endif // SENSOR_SDS011_H
