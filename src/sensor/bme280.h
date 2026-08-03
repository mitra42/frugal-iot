/* Frugal IoT - BME280 temperature, humidity and pressure sensor
 *
 * NOTE - AS OF 2026-08-03 THIS IS UNTESTED CODE
 *
 * Bosch BME280 over I2C. Freestanding, like Sensor_ms5803 - no external library, just
 * System_I2C - so nothing extra is compiled in and there is no third-party dependency to
 * track. The compensation arithmetic and the calibration unpacking are ported from Bosch's
 * own reference driver:
 *
 *   https://github.com/boschsensortec/BME280_SensorAPI  (bme280.c, BME280_DOUBLE_ENABLE)
 *   Copyright (c) 2023 Bosch Sensortec GmbH. SPDX-License-Identifier: BSD-3-Clause
 *
 * BSD-3-Clause is compatible with this library's MIT licence; the notice above is the
 * required attribution. Ported rather than reconstructed on purpose - the dig_h4/dig_h5
 * unpacking sign-extends the MSB *before* shifting, which is easy to get wrong and gives
 * plausible-but-incorrect humidity if you do.
 *
 * Why double and not float: the pressure polynomial divides by constants up to
 * 2147483648.0, which a 24-bit float mantissa cannot carry without losing accuracy. It runs
 * once per read cycle, so the cost of software double on ESP32/ESP8266 is irrelevant here.
 *
 * BME280 only - a BMP280 reports chip id 0x58 and has no humidity sensor, so it cannot sit
 * under Sensor_HT. setup() rejects anything that is not 0x60.
 *
 * Outputs published to MQTT:
 *   bme280/temperature - degrees C
 *   bme280/humidity    - % relative
 *   bme280/pressure    - hPa (Bosch returns Pa; converted here)
 *
 * Altitude is deliberately not published - it is a re-expression of pressure against an
 * assumed sea-level reference, so it is better derived downstream from bme280/pressure.
 *
 * Required: nothing beyond the Frugal-IoT library itself
 * Optional:
 *   SENSOR_BME280_ADDRESS (0x76) - 0x77 if SDO is tied high
 *   SENSOR_BME280_DEBUG          - Serial debug output
 *
 * Reads in Bosch's "weather monitoring" configuration - forced mode, 1x oversampling on all
 * three channels, IIR filter off. The chip sleeps between reads, which is what we want on a
 * node that takes one reading per wake cycle.
 */

#ifndef SENSOR_BME280_H
#define SENSOR_BME280_H

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "system/i2c.h"
#include "sensor/ht.h"

#ifndef SENSOR_BME280_ADDRESS
  #define SENSOR_BME280_ADDRESS 0x76
#endif

class Sensor_BME280 : public Sensor_HT {
  public:
    Sensor_BME280(const char * const name, uint8_t address = SENSOR_BME280_ADDRESS,
      TwoWire* wire = &I2C_WIRE, bool retain = true);
  protected:
    OUTfloat* pressure;
    System_I2C interface; // I2C object - compare Sensor_ms5803
    TwoWire* wire;
    bool present = false; // Set by the chip id check in setup()

    // Calibration coefficients, named as in the Bosch reference driver
    uint16_t dig_t1 = 0;
    int16_t  dig_t2 = 0, dig_t3 = 0;
    uint16_t dig_p1 = 0;
    int16_t  dig_p2 = 0, dig_p3 = 0, dig_p4 = 0, dig_p5 = 0, dig_p6 = 0, dig_p7 = 0, dig_p8 = 0, dig_p9 = 0;
    uint8_t  dig_h1 = 0, dig_h3 = 0;
    int16_t  dig_h2 = 0, dig_h4 = 0, dig_h5 = 0;
    int8_t   dig_h6 = 0;
    int32_t  t_fine = 0; // Set by compensateTemperature, consumed by pressure and humidity

    void setup() override;
    void readValidateConvertSet() override;
    void captiveLines(AsyncResponseStream* response) override;

    bool readCalibration();
    // Ported from Bosch bme280.c - see the licence note at the top of this file
    double compensateTemperature(int32_t raw); // Must be called before the other two
    double compensatePressure(int32_t raw);    // Returns Pa
    double compensateHumidity(int32_t raw);
    bool validate(float temp, float humy, float press);
};

#endif // SENSOR_BME280_H
