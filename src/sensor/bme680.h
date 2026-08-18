/* Frugal IoT - BME680 temperature, humidity, pressure and gas sensor
 *
 * NOTE - AS OF 2026-08-11 THIS IS UNTESTED CODE
 *
 * Bosch BME680 over I2C. Freestanding, like Sensor_BME280 and Sensor_ms5803 - no external
 * library, just System_I2C - so nothing extra is compiled in and there is no third-party
 * dependency to track. The compensation arithmetic, the calibration unpacking and the heater
 * register arithmetic are ported from Bosch's own reference driver:
 *
 *   https://github.com/boschsensortec/BME68x-Sensor-API  (bme68x.c, BME68X_USE_FPU)
 *   Copyright (c) 2023 Bosch Sensortec GmbH. SPDX-License-Identifier: BSD-3-Clause
 *
 * BSD-3-Clause is compatible with this library's MIT licence; the notice above is the
 * required attribution. (Note this is a *different* upstream repo to the one bme280.cpp cites
 * - the older BME680_driver repo is gone, and BME68x-Sensor-API is Bosch's replacement,
 * covering the BME680 and the BME688.)
 *
 * Why float here when bme280.cpp deliberately uses double: the reference implementation for
 * this chip *is* floating point single precision (Bosch's BME68X_USE_FPU variant, which is
 * their default) rather than the double variant that bme280.cpp ports. Matching it exactly
 * means the port can be - and was - checked bit-for-bit against Bosch's own functions on the
 * host, which is worth more than the fraction of a pascal the wider mantissa would buy.
 *
 * BME680 and BME688 are both accepted: they share chip id 0x61 and differ in the variant id
 * at register 0xF0 (0x00 = BME680, 0x01 = BME688), which selects one of two quite different
 * gas-resistance formulas and a different run_gas bit. Reading the wrong one gives a
 * plausible-but-wrong resistance rather than an obvious failure, so the variant is read in
 * setup() and honoured on every read.
 *
 * Outputs published to MQTT:
 *   bme680/temperature - degrees C
 *   bme680/humidity    - % relative
 *   bme680/pressure    - hPa (Bosch returns Pa; converted here)
 *   bme680/gas         - kOhm (Bosch returns ohms; converted here), omitted if gas is disabled
 *
 * Gas resistance is *not* an air quality index. It is the resistance of a heated metal-oxide
 * plate, which falls as the concentration of reducing (VOC) gases rises - higher is cleaner
 * air - but the absolute value drifts with humidity, temperature and sensor age. Turning it
 * into an IAQ number needs Bosch's BSEC, which ships as a closed-source binary blob for
 * specific architectures and is not redistributable under this licence, so it is deliberately
 * not attempted here. Use the trend, or derive an index downstream from bme680/gas.
 *
 * Altitude is deliberately not published either - it is a re-expression of pressure against an
 * assumed sea-level reference, so it is better derived downstream from bme680/pressure.
 *
 * Required: nothing beyond the Frugal-IoT library itself
 * Optional:
 *   SENSOR_BME680_ADDRESS (0x76)          - 0x77 if SDO is tied high
 *   SENSOR_BME680_HEATER_TEMP_C (320)     - gas plate target temperature, capped at 400 by the chip
 *   SENSOR_BME680_HEATER_MS (150)         - how long to hold it before sampling the plate
 *   SENSOR_BME680_MEASURE_TIMEOUT_MS(500) - give up waiting for a conversion
 *   SENSOR_BME680_DEBUG                   - Serial debug output
 *
 * Reads the temperature/humidity/pressure channels in Bosch's "weather monitoring"
 * configuration - forced mode, 1x oversampling on all three, IIR filter off - so the chip
 * sleeps between reads, which is what we want on a node taking one reading per wake cycle.
 *
 * Power: the gas heater is the expensive part - order 12mA for HEATER_MS on every read, and
 * the read blocks for that long too (roughly 12ms of TPH conversion plus the heater time).
 * Pass gas=false to the constructor on a battery node that only wants temperature, humidity
 * and pressure: the heater is then left off, no time is spent waiting for it, and the gas
 * output is not created at all rather than published as a value that never changes.
 */

#ifndef SENSOR_BME680_H
#define SENSOR_BME680_H

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "system/i2c.h"
#include "sensor/ht.h"

#ifndef SENSOR_BME680_ADDRESS
  #define SENSOR_BME680_ADDRESS 0x76
#endif
#ifndef SENSOR_BME680_HEATER_TEMP_C
  #define SENSOR_BME680_HEATER_TEMP_C 320 // Bosch's own default profile is 320C for 150ms
#endif
#ifndef SENSOR_BME680_HEATER_MS
  #define SENSOR_BME680_HEATER_MS 150
#endif
#ifndef SENSOR_BME680_MEASURE_TIMEOUT_MS
  #define SENSOR_BME680_MEASURE_TIMEOUT_MS 500
#endif

class Sensor_BME680 : public Sensor_HT {
  public:
    Sensor_BME680(const char * const name, uint8_t address = SENSOR_BME680_ADDRESS,
      TwoWire* wire = &I2C_WIRE, bool retain = true, bool wantGas = true);
  protected:
    OUTfloat* pressure;
    OUTfloat* gas;        // nullptr if the constructor was passed gas=false
    System_I2C interface; // I2C object - compare Sensor_BME280 and Sensor_ms5803
    TwoWire* wire;
    bool present = false;    // Set by the chip id check in setup()
    uint8_t variant = 0;     // 0x00 = BME680 (gas low), 0x01 = BME688 (gas high)
    int8_t ambient_c = 25;   // Feeds the heater resistance sum; updated from each good reading

    // Calibration coefficients, named as in the Bosch reference driver
    uint16_t par_t1 = 0;
    int16_t  par_t2 = 0;
    int8_t   par_t3 = 0;
    uint16_t par_p1 = 0;
    int16_t  par_p2 = 0, par_p4 = 0, par_p5 = 0, par_p8 = 0, par_p9 = 0;
    int8_t   par_p3 = 0, par_p6 = 0, par_p7 = 0;
    uint8_t  par_p10 = 0;
    uint16_t par_h1 = 0, par_h2 = 0;
    int8_t   par_h3 = 0, par_h4 = 0, par_h5 = 0, par_h7 = 0;
    uint8_t  par_h6 = 0;
    int8_t   par_gh1 = 0, par_gh3 = 0;
    int16_t  par_gh2 = 0;
    uint8_t  res_heat_range = 0;
    int8_t   res_heat_val = 0, range_sw_err = 0;
    float    t_fine = 0;  // Set by compensateTemperature, consumed by pressure and humidity

    void setup() override;
    void readValidateConvertSet() override;
    void captiveLines(AsyncResponseStream* response) override;

    bool readCalibration();
    void configureMeasurement(); // Heater + oversampling registers, rewritten before each read
    // Ported from Bosch bme68x.c - see the licence note at the top of this file
    float compensateTemperature(uint32_t raw); // Must be called before the other two
    float compensatePressure(uint32_t raw);    // Returns Pa
    float compensateHumidity(uint16_t raw);
    float compensateGasLow(uint16_t raw, uint8_t range);  // BME680, returns ohms
    float compensateGasHigh(uint16_t raw, uint8_t range); // BME688, returns ohms
    uint8_t calcResHeat(uint16_t temp_c);      // Heater target temperature -> res_heat_0
    uint8_t calcGasWait(uint16_t dur_ms);      // Heater duration -> gas_wait_0
    bool validate(float temp, float humy, float press);
    bool validateGas(float kohm);
};

#endif // SENSOR_BME680_H
