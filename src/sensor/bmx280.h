/* Frugal IoT - BMP280 / BME280 temperature, pressure (and on the BME280, humidity) sensor
 *
 * NOTE - AS OF 2026-08-18 THIS IS UNTESTED CODE
 *
 * Two Bosch chips over I2C, sharing one class. Freestanding, like Sensor_ms5803 - no external
 * library, just System_I2C - so nothing extra is compiled in and there is no third-party
 * dependency to track. The compensation arithmetic and the calibration unpacking are ported
 * from Bosch's own reference driver:
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
 * Why one class for both chips
 * ────────────────────────────
 * A BME280 is a BMP280 plus a humidity channel, and that is true right down to the register
 * map: same chip-id/reset/status/ctrl_meas/config registers, same 26-byte temperature and
 * pressure calibration block at 0x88, same forced-mode sequence, same 20-bit raw layout. The
 * BME280 adds a 7-byte humidity calibration block at 0xE1, the ctrl_hum register at 0xF2, and
 * two more data bytes. So the shared class carries the whole thing and `humidity` is a null
 * pointer on a BMP280 - the same "absent output is a nullptr" pattern Sensor_BME680 uses for
 * its gas channel. There is no separate Sensor_HT base class any more (it did little beyond
 * constructing these two outputs, and a BMP280 could not have used it).
 *
 * The chips are told apart by their chip id, and this is not just belt-and-braces: the two
 * boards look identical, are sold interchangeably, and a BME280 driven as a BMP280 would
 * publish perfectly plausible temperature and pressure while silently having no humidity. So
 * setup() rejects the wrong chip rather than adapting to it - if you have the other one, say
 * so in the sketch.
 *
 *   Sensor_BMP280 - chip id 0x58 (also 0x56/0x57, Bosch's engineering samples)
 *   Sensor_BME280 - chip id 0x60
 *   (0x61 is a BME680/BME688 - different register map entirely, see sensor/bme680.h)
 *
 * Outputs published to MQTT:
 *   bmp280/temperature  bme280/temperature - degrees C
 *   bmp280/pressure     bme280/pressure    - hPa (Bosch returns Pa; converted here)
 *                       bme280/humidity    - % relative (BME280 only)
 *
 * Altitude is deliberately not published - it is a re-expression of pressure against an
 * assumed sea-level reference, so it is better derived downstream from <id>/pressure.
 *
 * Required: nothing beyond the Frugal-IoT library itself
 * Optional:
 *   SENSOR_BME280_ADDRESS (0x76) - 0x77 if SDO is tied high
 *   SENSOR_BMP280_ADDRESS (0x76) - as above; the BMP280's alternate address is also 0x77
 *   SENSOR_BME280_DEBUG          - Serial debug output
 *   SENSOR_BMP280_DEBUG          - as above (either flag turns on the shared debug output)
 *
 * Reads in Bosch's "weather monitoring" configuration - forced mode, 1x oversampling on all
 * channels, IIR filter off. The chip sleeps between reads, which is what we want on a node
 * that takes one reading per wake cycle.
 */

#ifndef SENSOR_BMX280_H
#define SENSOR_BMX280_H

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "system/i2c.h"
#include "sensor/sensor.h"

#ifndef SENSOR_BME280_ADDRESS
  #define SENSOR_BME280_ADDRESS 0x76
#endif
#ifndef SENSOR_BMP280_ADDRESS
  #define SENSOR_BMP280_ADDRESS 0x76
#endif

class Sensor_BMx280 : public Sensor {
  public:
    // Not instantiated directly - use Sensor_BMP280 or Sensor_BME280 below
    // humidityRange is ignored on a BMP280, which passes wantHumidity = false
    Sensor_BMx280(const char * const id, const char * const name, uint8_t chipId,
      uint8_t address, TwoWire* wire, bool retain, bool wantHumidity,
      const OutputRange temperatureRange, const OutputRange pressureRange, const OutputRange humidityRange);
    OUTfloat* temperature;
    OUTfloat* pressure;
    OUTfloat* humidity;   // nullptr on a BMP280 - the presence of the output is the flag
  protected:
    System_I2C interface; // I2C object - compare Sensor_ms5803
    TwoWire* wire;
    const uint8_t chipId; // What setup() insists on reading back from register 0xD0
    bool present = false;  // Set by the chip id check in setup()

    // Calibration coefficients, named as in the Bosch reference driver
    uint16_t dig_t1 = 0;
    int16_t  dig_t2 = 0, dig_t3 = 0;
    uint16_t dig_p1 = 0;
    int16_t  dig_p2 = 0, dig_p3 = 0, dig_p4 = 0, dig_p5 = 0, dig_p6 = 0, dig_p7 = 0, dig_p8 = 0, dig_p9 = 0;
    uint8_t  dig_h1 = 0, dig_h3 = 0;      // Humidity coefficients stay zero on a BMP280
    int16_t  dig_h2 = 0, dig_h4 = 0, dig_h5 = 0;
    int8_t   dig_h6 = 0;
    int32_t  t_fine = 0; // Set by compensateTemperature, consumed by pressure and humidity

    void setup() override;
    void readValidateConvertSet() override;

    virtual bool chipIdValid(uint8_t readId); // BMP280 widens this to its engineering samples
    bool readCalibration();
    // Ported from Bosch bme280.c - see the licence note at the top of this file
    double compensateTemperature(int32_t raw); // Must be called before the other two
    double compensatePressure(int32_t raw);    // Returns Pa
    double compensateHumidity(int32_t raw);
    bool validate(float temp, float press, float humy); // humy is NAN on a BMP280
};

class Sensor_BMP280 : public Sensor_BMx280 {
  public:
    Sensor_BMP280(const char * const name, uint8_t address = SENSOR_BMP280_ADDRESS,
      TwoWire* wire = &I2C_WIRE, bool retain = true);
  protected:
    bool chipIdValid(uint8_t readId) override; // 0x58, plus Bosch's 0x56/0x57 samples
};

class Sensor_BME280 : public Sensor_BMx280 {
  public:
    Sensor_BME280(const char * const name, uint8_t address = SENSOR_BME280_ADDRESS,
      TwoWire* wire = &I2C_WIRE, bool retain = true);
};

#endif // SENSOR_BMX280_H
