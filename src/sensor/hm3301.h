// Deep Sleep issues: as Sensor_PM (sensor/pm.h) - the node is held awake while it reads. The HM3301 has no sleep, so its fan runs through the ESP's deep sleep; a deep-sleep wake therefore skips the warm-up (about 5s awake), and only a power-on waits SENSOR_HM3301_WARMUP_MS (about 35s).
/*
 * Seeed HM3301 laser particulate sensor (sold as "Grove - Laser PM2.5 Sensor (HM3301)") - PM1.0,
 * PM2.5 and PM10 over I2C.
 *
 * NOTE - AS OF 2026-11-06 THIS IS UNTESTED CODE - written from Seeed's library and wiki, not yet run
 * against a module. Bug reports or confirmation that it works very welcome.
 *
 * Freestanding - no external library. Seeed's (github.com/Seeed-Studio/Seeed_PM2_5_sensor_HM3301,
 * MIT) is where the protocol below comes from.
 *
 * Protocol (I2C, address 0x40):
 *   Once after power-on, write the one byte 0x88 to select I2C (the module also speaks UART).
 *   Then read 29 bytes whenever a reading is wanted - the module measures continuously:
 *     bytes 0-1   reserved           bytes 2-3   sensor number
 *     bytes 4-9   PM1.0, PM2.5, PM10 "standard particulate" (CF=1) - for factory calibration
 *     bytes 10-15 PM1.0, PM2.5, PM10 "atmospheric environment"    - what is published here
 *     bytes 16-27 not used here      byte 28     checksum = low byte of the sum of bytes 0-27
 *   Each value is big-endian, whole ug/m3.
 *
 * How a reading is taken: Sensor_PM's cycle - see sensor/pm.h - sampling once every
 * SENSOR_HM3301_SAMPLE_MS while READING. The difference from the SDS011 is that the HM3301 has
 * no sleep: its fan and laser run whenever it has power. So wakeModule() does nothing, and
 * sleepModule() says it could not sleep, which leaves `running` true. After a deep-sleep wake
 * setup() sets `running` too, as the fan ran right through the sleep - so only the first
 * measurement after a power-on has a warm-up.
 * Do not give the HM3301 switched power through powerPins(): powerPins() on any I2C sensor
 * switches the whole bus, and this class would then wrongly assume the fan had kept running.
 *
 * Module:
 *   Supply  3.3V or 5V, according to Seeed. 3.3V is the safer choice on an ESP, as the Grove
 *           board's I2C pull-ups go to whatever it is supplied with.
 *   Stable  30s after power-on, according to Seeed - hence the warm-up default.
 *   Life    the fan runs continuously, which wears it and the laser out faster than a module
 *           that sleeps (see Sensor_SDS011). Switching its supply is a possible future addition.
 *
 * Output published to MQTT:
 *   hm3301/pm1    - PM1.0 in ug/m3
 *   hm3301/pm25   - PM2.5 in ug/m3
 *   hm3301/pm10   - PM10 in ug/m3
 *
 * Build flags:
 *   SENSOR_HM3301_ADDRESS (0x40)        - fixed in the module; INA219s and HDC1080s use it too
 *   SENSOR_HM3301_WARMUP_MS (30000)     - after a power-on only - see above
 *   SENSOR_HM3301_READING_MS (5000)     - samples averaged
 *   SENSOR_HM3301_SAMPLE_MS (1000)      - between samples while reading
 *   SENSOR_HM3301_DEBUG                 - Serial debug output
 */

#ifndef SENSOR_HM3301_H
#define SENSOR_HM3301_H

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "system/i2c.h"
#include "sensor/pm.h"

#ifndef SENSOR_HM3301_ADDRESS
  #define SENSOR_HM3301_ADDRESS 0x40
#endif
#ifndef SENSOR_HM3301_WARMUP_MS
  #define SENSOR_HM3301_WARMUP_MS 30000 // Seeed: "stable 30 seconds after power-on"
#endif
#ifndef SENSOR_HM3301_READING_MS
  #define SENSOR_HM3301_READING_MS 5000 // As the SDS011 - five samples
#endif
#ifndef SENSOR_HM3301_SAMPLE_MS
  #define SENSOR_HM3301_SAMPLE_MS 1000
#endif

// TODO - remove once defaults.h is regenerated from modules.yaml, which now has hm3301. Until then
// these stand in, and then defaults.h (included via _settings.h) takes over.
#ifndef DEFAULT_hm3301_pm1_min
  #define DEFAULT_hm3301_pm1_min 0
  #define DEFAULT_hm3301_pm1_max 100
  #define DEFAULT_hm3301_pm1_color "#d2691e"
#endif
#ifndef DEFAULT_hm3301_pm25_min
  #define DEFAULT_hm3301_pm25_min 0
  #define DEFAULT_hm3301_pm25_max 100
  #define DEFAULT_hm3301_pm25_color "#a0522d"
#endif
#ifndef DEFAULT_hm3301_pm10_min
  #define DEFAULT_hm3301_pm10_min 0
  #define DEFAULT_hm3301_pm10_max 200
  #define DEFAULT_hm3301_pm10_color "#696969"
#endif

class Sensor_HM3301 : public Sensor_PM {
  public:
    Sensor_HM3301(const char * const name, uint8_t address = SENSOR_HM3301_ADDRESS,
      TwoWire* wire = &I2C_WIRE, bool retain = true);
  protected:
    System_I2C interface; // I2C object - compare Sensor_BMx280
    // powerPins() would apply to the shared bus, not to this object - see the warning above
    System_Interface* powerInterface() override { return interface.bus(); }
    unsigned long lastSample = 0;

    void setup() override;
    void wakeModule() override;
    bool sleepModule() override;
    bool readSample(uint16_t* v) override;
    void reportNoData() override;
};

#endif // SENSOR_HM3301_H
