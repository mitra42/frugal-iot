// Deep Sleep issues: none - read fresh on each wake.
/* Frugal IoT - SHT3x / SHT4x temperature and humidity sensor
 *
 * Sensirion SHT3x and SHT4x over I2C, freestanding - no external library, just System_I2C.
 * Previously this drove RobTillaart's SHT85 and SHT4x libraries, one or the other chosen at
 * compile time; the history and the lessons from those are kept in the notes below.
 *
 * Mitra Ardron: Sept 2024...Jun 2025
 *
 * See the guide to building one at
 * https://github.com/mitra42/frugal-iot/wiki/Building-a-temperature---humidity-sensor/
 *
 * Tested on:
 * Sensors: Lolin SHT30; a nice one on a cord, including some really cheap ones - no known
 *          compatability issues. SHT40 on the supermini boards.
 * Dev boards: ESP8266 & ESP32 on multiple boards - no known compatability issues
 *
 * Which chip is fitted, and which of its two addresses it answers on, are both worked out at
 * runtime - see Sensor_SHT::detect() and the SENSOR_SHT_ADDRESS note below. The two families
 * look identical, share the same pair of addresses, and both acknowledge their address, so a
 * node built for the wrong one reports a perfectly healthy bus and then never returns a
 * reading. That is a bad failure to hand somebody in a field, so the driver asks instead.
 *
 * Outputs published to MQTT:
 *   sht/temperature - degrees C
 *   sht/humidity    - % relative
 *
 * Required: nothing beyond the Frugal-IoT library itself
 * Optional:
 *   SENSOR_SHT_ADDRESS            - leave undefined to try 0x44 then 0x45; define it to pin the
 *                                   sensor to one address and have anything else be an error
 *   SENSOR_SHT_SHT4x              - no longer selects the driver, both are always built. Now
 *                                   only says "expect a 4x", which probes that family first
 *                                   and saves one I2C error line in the log at boot.
 *   SENSOR_SHT_TIMEOUT_MS (50)    - give up waiting for a conversion
 *   SENSOR_SHT_DEBUG              - Serial debug output
 *
 * TODO-16 Support multiple I2C Wires - so for example can use two sensors on each wire. See Issue#16
 * TODO Support I2C multiplexors - see sample code at https://github.com/RobTillaart/SHT85/issues/26#issuecomment-2367448245
*/

#ifndef SENSOR_SHT_H
#define SENSOR_SHT_H

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "system/i2c.h"
#include "sensor/sensor.h"

/* The I2C address, or SENSOR_SHT_ADDRESS_AUTO when nothing has said which.
 *
 * Both families use 0x44 or 0x45, selected by a link on the breakout - and the parts are
 * unlabelled, so on a sensor somebody has just been handed "one of those two" is genuinely
 * all that is known. So:
 *
 *   SENSOR_SHT_ADDRESS undefined  - setup() tries 0x44, then 0x45
 *   SENSOR_SHT_ADDRESS defined,
 *   or an address passed to the
 *   constructor                   - ONLY that address is tried, and not finding it is an error
 *
 * The second is what you want once a board is known: a mis-set link then gets reported instead
 * of silently working, which matters when the two addresses are two different sensors on one
 * bus. Has to be in .h, so it can be used in main.cpp's constructor call.
 */
#define SENSOR_SHT_ADDRESS_AUTO 0x00 // Not usable as a device address, so unambiguous as a flag

#ifndef SENSOR_SHT_ADDRESS
  // TODO build address this into OTA Key as requires two binaries
  #define SENSOR_SHT_ADDRESS SENSOR_SHT_ADDRESS_AUTO
#endif

/* How long to wait for a conversion before giving up.
 *
 * A high repeatability single shot takes up to 15.5ms on an SHT3x and 8.3ms on an SHT4x, so
 * this is generous - it exists to bound the wait when the sensor has stopped answering, not to
 * time a healthy one. See the polling note in readValidateConvertSet().
 */
#ifndef SENSOR_SHT_TIMEOUT_MS
  #define SENSOR_SHT_TIMEOUT_MS 50
#endif

// Default power control pins - can be overridden via constructor parameters
#ifndef SENSOR_SHT_POWER0_PIN
  #define SENSOR_SHT_POWER0_PIN 0xff
#endif
#ifndef SENSOR_SHT_POWER3v3_PIN
  #define SENSOR_SHT_POWER3v3_PIN 0xff
#endif

// Which family setup() found. SHT_FAMILY_NONE means nothing answered.
enum Sensor_SHT_Family { SHT_FAMILY_NONE, SHT_FAMILY_3x, SHT_FAMILY_4x };

class Sensor_SHT : public Sensor {
  public:
    Sensor_SHT(const char * const name, uint8_t address = SENSOR_SHT_ADDRESS,
      TwoWire *wire = &I2C_WIRE, bool retain = true);
    OUTfloat* temperature;
    OUTfloat* humidity;
  protected:
    System_I2C interface;
    // powerPins() applies to the shared bus, not to this object - see system/interface.h
    System_Interface* powerInterface() override { return interface.bus(); }
    Sensor_SHT_Family family = SHT_FAMILY_NONE;

    void setup() override;
    void readValidateConvertSet() override; // Combines function of set(read()) since read gets two values from sensor
    bool validate(float temp, float humy);
    // Ask the chip which family it is. Only meaningful once the bus is up and the sensor is
    // powered, so setup() calls it and nothing else should.
    Sensor_SHT_Family detect();
    bool probe3x(); // Ask for the status register - only an SHT3x answers
    bool probe4x(); // Ask for the serial number - only an SHT4x answers
    bool send3x(uint16_t cmd); // SHT3x commands are 16 bit; SHT4x are one byte via interface.send()
    /* Read `words` 16-bit words, each followed by its own CRC, and check them. This is the
     * wire format of everything either family sends back - status, serial number and
     * measurement alike - so it is the only read path here. False means the device NACKed
     * (usually "still converting") or the CRC did not match.
     */
    bool readWords(uint16_t* out, uint8_t words);
    static uint8_t crc8(const uint8_t* data, uint8_t len);
};

#endif // SENSOR_SHT_H
