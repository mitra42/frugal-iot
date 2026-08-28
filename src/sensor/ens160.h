/* Frugal IoT - ENS160 air quality sensor (AQI, TVOC, eCO2)
 *
 * ScioSense ENS160 (and ENS161) over I2C, freestanding - no external library, just System_I2C.
 *
 * This is the ENS half of what used to be sensor/ens160aht21.cpp, the driver for the common
 * "ENS160+AHT21" breakout, which drove both chips from one class. The AHT is now
 * sensor/aht.h and the two are joined in the sketch instead - see examples/ensaht:
 *
 *     Sensor_AHT21* aht = new Sensor_AHT21("AHT21");
 *     Sensor_ENS160* ens = new Sensor_ENS160("ENS160");
 *     frugal_iot.sensors->add(aht);
 *     frugal_iot.sensors->add(ens);   // wires itself to aht21/temperature and aht21/humidity
 *
 * Why the split. The ENS160 needs an ambient temperature and humidity to compensate its gas
 * plate, but it does not care where they come from - the AHT21 that happens to share the
 * breakout, an SHT30 elsewhere on the node, or a reading published by a different node
 * entirely. Welding the two chips into one class made the AHT21 unusable on its own, made the
 * ENS160 unusable without one, and hid the temperature/humidity dependency from the UX. So the
 * compensation values arrive as `IN`s, wired like any other signal - the same shape as
 * Sensor_DissolvedOxygen's water temperature input. They default to the AHT21's topics, so the
 * common breakout still works with no wiring in the sketch.
 *
 * Outputs published to MQTT:
 *   ens160/aqi     - air quality index, 1 (excellent) to 5 (unhealthy)
 *   ens160/tvoc    - total volatile organic compounds, ppb
 *   ens160/eco2    - equivalent CO2, ppm - *equivalent*: it is derived from the VOC reading,
 *                    not measured, so it will not see CO2 from breathing without VOCs present
 *   ens160/aqi500  - 0..500 scale index, ENS161 only; the output is dropped in setup() on an
 *                    ENS160 rather than published as a value that never changes
 *
 * Inputs (both wireable, and both wired by default to the AHT21 on the combined board):
 *   ens160/temperature - ambient temperature, C
 *   ens160/humidity    - ambient relative humidity, %
 *
 * Required: nothing beyond the Frugal-IoT library itself
 * Optional:
 *   SENSOR_ENS160_ADDRESS (0x53)              - alternate is 0x52
 *   SENSOR_ENS160_TEMPERATURE_PATH ("aht21/temperature")
 *   SENSOR_ENS160_HUMIDITY_PATH ("aht21/humidity")
 *   SENSOR_ENS160_DEFAULT_TEMPERATURE (25)    - assumed until the first message arrives
 *   SENSOR_ENS160_DEFAULT_HUMIDITY (50)
 *   SENSOR_ENS160_TIMEOUT_MS (1000)           - give up waiting for a measurement
 *   SENSOR_ENS160_DEBUG                       - Serial debug output
 *
 * Based on https://github.com/adafruit/ENS160_driver - which implements more than is used here,
 * especially custom heater profiles; see also the extra commands in its ScioSense_ENS160.h
 *
 * Some info found online for the combined board:
 * Info: https://www.instructables.com/ENS160-AHT21-Sensor-for-Arduino/
 * Issue: https://github.com/mitra42/frugal-iot/issues/101
 * Reddit: https://www.reddit.com/r/arduino/comments/12ulwo2/has_anyone_been_able_to_get_ensaht_working/
 * One of the articles says Vin is 5V - dont use 3.3V its an output from the regulator, but I
 * had it work on 3V fine.
 *
 * //TODO-101 and TODO-23 note ENS160_OPMODE_DEP_SLEEP is not used - the chip stays in standard
 * mode between reads, which costs current on a battery node.
 */

#ifndef SENSOR_ENS160_H
#define SENSOR_ENS160_H

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "system/i2c.h"
#include "sensor/sensor.h"

#ifndef SENSOR_ENS160_ADDRESS
  #define SENSOR_ENS160_ADDRESS 0x53 // Alternates are 0x52 0x53
#endif
#ifndef SENSOR_ENS160_TEMPERATURE_PATH
  #define SENSOR_ENS160_TEMPERATURE_PATH "aht21/temperature"
#endif
#ifndef SENSOR_ENS160_HUMIDITY_PATH
  #define SENSOR_ENS160_HUMIDITY_PATH "aht21/humidity"
#endif
#ifndef SENSOR_ENS160_DEFAULT_TEMPERATURE
  #define SENSOR_ENS160_DEFAULT_TEMPERATURE 25.0 // Until something arrives on the input
#endif
#ifndef SENSOR_ENS160_DEFAULT_HUMIDITY
  #define SENSOR_ENS160_DEFAULT_HUMIDITY 50.0
#endif
#ifndef SENSOR_ENS160_TIMEOUT_MS
  #define SENSOR_ENS160_TIMEOUT_MS 1000 // A conversion is ~1s in standard mode
#endif

class Sensor_ENS160 : public Sensor {
  public:
    Sensor_ENS160(const char * const name, uint8_t address = SENSOR_ENS160_ADDRESS,
      TwoWire* wire = &I2C_WIRE, bool retain = false);
    // Compensation inputs - wired in setup() to the AHT21 unless something already wired them
    INfloat* temperature;
    INfloat* humidity;
  protected:
    OUTuint16* aqi;
    OUTuint16* tvoc;
    OUTuint16* eco2;
    OUTuint16* aqi500;      // ENS161 only - deleted in setup() on an ENS160
    System_I2C interface;
    bool present = false;   // Set by the part id read in setup()
    bool isENS161 = false;  // aqi500 is only on the ENS161

    void setup() override;
    void discover() override;
    void dispatch(System_Message &msg) override;
    void readValidateConvertSet() override;
    void captiveLines(AsyncResponseStream* response) override; // Adds the compensation inputs

    bool sendAndWait(uint8_t reg, uint8_t val); // Register write plus the chip's settling delay
    bool setMode(uint8_t val);
    bool command(uint8_t val);
    bool sendAndRead(uint8_t reg, uint8_t *buf, uint8_t num);
    bool setEnvData(float temp, float humy);   // Feed the compensation inputs to the chip
};

#endif // SENSOR_ENS160_H
