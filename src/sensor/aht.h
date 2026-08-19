/* Frugal IoT - AHT20 / AHT21 temperature and humidity sensor
 *
 * NOTE - the AHT21 path is the code that used to live inside sensor/ens160aht21.cpp and has run
 * on the combined ENS160+AHT21 board; the AHT20 subclass is UNTESTED as of 2026-08-18.
 *
 * Aosong AHTxx over I2C, freestanding - no external library, just System_I2C. Lessons learned
 * and some ideas/bits copied from https://github.com/adafruit/Adafruit_AHTX0 (MIT).
 *
 * Why one base class and two near-empty subclasses
 * ───────────────────────────────────────────────
 * The AHT20 and the AHT21 are the same device as far as this driver is concerned: same I2C
 * address, same soft-reset/trigger/status commands, same 20-bit humidity-then-temperature data
 * layout, same conversion formulas. They differ in packaging, in the AHT21's tighter humidity
 * tolerance, and in one initialisation detail (see SENSOR_AHT_CMD_INIT below) - none of which
 * changes a line of code here. So the subclasses exist only to give each chip its own module id
 * (`aht20/temperature` vs `aht21/temperature`), which is what the UX and the MQTT topics key
 * off, and to make a sketch say which chip is actually fitted.
 *
 * Outputs published to MQTT:
 *   aht20/temperature  aht21/temperature - degrees C
 *   aht20/humidity     aht21/humidity    - % relative
 *
 * Required: nothing beyond the Frugal-IoT library itself
 * Optional:
 *   SENSOR_AHT_ADDRESS (0x38)    - the only address these parts use, but some clones offer 0x39
 *   SENSOR_AHT_CMD_INIT (0xE1)   - see below
 *   SENSOR_AHT_TIMEOUT_MS (200)  - give up waiting for the chip to leave its busy state
 *   SENSOR_AHT_DEBUG             - Serial debug output
 *
 * SENSOR_AHT_CMD_INIT is 0xE1 - Adafruit's value, and what the ENS160+AHT21 board this code
 * came from has always been sent. The AHT20 and AHT21 datasheets both specify 0xBE for that
 * command (0xE1 is the AHT10's), and these parts are factory calibrated either way, so the
 * command is close to a no-op in practice. It is a #define so that a board that does object can
 * be given 0xBE without touching the driver.
 *
 * The ENS160 that ships on the same little board is a *separate* sensor - see sensor/ens160.h,
 * and examples/ensaht for the two of them wired together (the ENS160 needs this chip's
 * temperature and humidity for its compensation).
 */

#ifndef SENSOR_AHT_H
#define SENSOR_AHT_H

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "system/i2c.h"
#include "sensor/sensor.h"

#ifndef SENSOR_AHT_ADDRESS
  #define SENSOR_AHT_ADDRESS 0x38 // Alternates are 0x38 0x39 on some clones
#endif
#ifndef SENSOR_AHT_CMD_INIT
  #define SENSOR_AHT_CMD_INIT 0xE1 // See the note above - the datasheets say 0xBE
#endif
#ifndef SENSOR_AHT_TIMEOUT_MS
  #define SENSOR_AHT_TIMEOUT_MS 200 // A measurement takes ~80ms; a missing chip takes forever
#endif

class Sensor_AHT : public Sensor {
  public:
    // Not instantiated directly - use Sensor_AHT20 or Sensor_AHT21 below
    Sensor_AHT(const char * const id, const char * const name, uint8_t address,
      TwoWire* wire, bool retain, const OutputRange temperatureRange, const OutputRange humidityRange);
    OUTfloat* temperature;
    OUTfloat* humidity;
  protected:
    System_I2C interface;
    bool present = false; // Set in setup() by the first status read

    void setup() override;
    void readValidateConvertSet() override;
    // Waits out the busy bit, returning the last status read. `ok` is false if it gave up -
    // note the old ens160aht21 version had no timeout and spun forever on a missing chip,
    // which on this platform means a watchdog reset rather than an error.
    uint8_t spinTillReady(bool &ok);
    bool validate(float temp, float humy);
};

class Sensor_AHT20 : public Sensor_AHT {
  public:
    Sensor_AHT20(const char * const name, uint8_t address = SENSOR_AHT_ADDRESS,
      TwoWire* wire = &I2C_WIRE, bool retain = true);
};

class Sensor_AHT21 : public Sensor_AHT {
  public:
    Sensor_AHT21(const char * const name, uint8_t address = SENSOR_AHT_ADDRESS,
      TwoWire* wire = &I2C_WIRE, bool retain = true);
};

#endif // SENSOR_AHT_H
