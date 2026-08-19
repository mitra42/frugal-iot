/* Frugal IoT - BMP280 / BME280 temperature, pressure (and on the BME280, humidity) sensor
 *
 * NOTE - AS OF 2026-08-18 THIS IS UNTESTED CODE
 *
 * See sensor/bmx280.h for the licence attribution, register configuration and why both chips
 * share one class.
 *
 * Compensation and calibration unpacking ported from Bosch's reference driver:
 *   https://github.com/boschsensortec/BME280_SensorAPI  (bme280.c, BME280_DOUBLE_ENABLE)
 *   Copyright (c) 2023 Bosch Sensortec GmbH. SPDX-License-Identifier: BSD-3-Clause
 */

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "sensor/bmx280.h"
#include "cmath" // for std::isnan

// Register map, per the Bosch reference driver. Every one of these is at the same address on
// the BMP280 except CTRL_HUM and HUMIDITY_CALIB, which that chip does not have.
#define BMX280_REG_CHIP_ID              0xD0
#define BMX280_REG_RESET                0xE0
#define BMX280_REG_TEMP_PRESS_CALIB     0x88
#define BMX280_REG_HUMIDITY_CALIB       0xE1
#define BMX280_REG_CTRL_HUM             0xF2
#define BMX280_REG_STATUS               0xF3
#define BMX280_REG_CTRL_MEAS            0xF4
#define BMX280_REG_CONFIG               0xF5
#define BMX280_REG_DATA                 0xF7

#define BMP280_CHIP_ID                  0x58 // Production BMP280
#define BMP280_CHIP_ID_SAMPLE1          0x56 // Bosch engineering samples, accepted by their driver
#define BMP280_CHIP_ID_SAMPLE2          0x57
#define BME280_CHIP_ID                  0x60 // A BME680/BME688 answers 0x61 - different register map
#define BMX280_CMD_RESET                0xB6
#define BMX280_LEN_TEMP_PRESS_CALIB     26
#define BMX280_LEN_HUMIDITY_CALIB       7
#define BMX280_LEN_DATA_NO_HUMIDITY     6  // press[3] + temp[3]
#define BMX280_LEN_DATA                 8  // ... + humidity[2]
#define BMX280_STATUS_MEASURING         0x08

// Bosch "weather monitoring": forced mode, 1x oversampling everywhere, IIR filter off.
#define BMX280_CTRL_HUM_X1              0x01 // osrs_h = 001
#define BMX280_CTRL_MEAS_FORCED_X1      0x25 // osrs_t = 001, osrs_p = 001, mode = 01 (forced)
#define BMX280_CONFIG_FILTER_OFF        0x00
#define BMX280_MEASURE_TIMEOUT_MS       50   // Max measurement time at 1x is ~8ms

#define BMX280_CONCAT_BYTES(msb, lsb)   (((uint16_t)msb << 8) | (uint16_t)lsb)

Sensor_BMx280::Sensor_BMx280(const char * const id, const char * const name, uint8_t chipId,
  uint8_t address, TwoWire* wire, bool retain, bool wantHumidity,
  const OutputRange temperatureRange, const OutputRange pressureRange, const OutputRange humidityRange)
  : Sensor(id, name, retain),
    humidity(nullptr), // Stays null on a BMP280 - the presence of the output is the flag
    interface(address, wire),
    wire(wire),
    chipId(chipId)
{
  // The ranges come from the subclass, so each chip gets its own module's UX defaults
  outputs.push_back(temperature = new OUTfloat(id, "temperature", "Temperature", 0, 1, temperatureRange.min, temperatureRange.max, temperatureRange.color, false));
  outputs.push_back(pressure = new OUTfloat(id, "pressure", "Pressure", 0, 1, pressureRange.min, pressureRange.max, pressureRange.color, false));
  temperature->unit = "C";
  pressure->unit = "hPa";
  if (wantHumidity) {
    outputs.push_back(humidity = new OUTfloat(id, "humidity", "Humidity", 0, 1, humidityRange.min, humidityRange.max, humidityRange.color, false));
    humidity->unit = "%";
  }
}

// Only the exact production id by default; Sensor_BMP280 widens it.
bool Sensor_BMx280::chipIdValid(uint8_t readId) {
  return readId == chipId;
}

// 26 bytes from 0x88 (dig_t1..dig_p9 then dig_h1), then - on a BME280 only - 7 bytes from
// 0xE1 (dig_h2..dig_h6). dig_h4 and dig_h5 share the nibbles of the byte at 0xE5; note the
// sign extension of the MSB happens before the shift, which is what makes this worth porting
// rather than guessing.
bool Sensor_BMx280::readCalibration() {
  uint8_t tp[BMX280_LEN_TEMP_PRESS_CALIB];
  bool ok = interface.sendAndRead(BMX280_REG_TEMP_PRESS_CALIB, tp, BMX280_LEN_TEMP_PRESS_CALIB);
  if (ok) {
    dig_t1 = BMX280_CONCAT_BYTES(tp[1], tp[0]);
    dig_t2 = (int16_t)BMX280_CONCAT_BYTES(tp[3], tp[2]);
    dig_t3 = (int16_t)BMX280_CONCAT_BYTES(tp[5], tp[4]);
    dig_p1 = BMX280_CONCAT_BYTES(tp[7], tp[6]);
    dig_p2 = (int16_t)BMX280_CONCAT_BYTES(tp[9], tp[8]);
    dig_p3 = (int16_t)BMX280_CONCAT_BYTES(tp[11], tp[10]);
    dig_p4 = (int16_t)BMX280_CONCAT_BYTES(tp[13], tp[12]);
    dig_p5 = (int16_t)BMX280_CONCAT_BYTES(tp[15], tp[14]);
    dig_p6 = (int16_t)BMX280_CONCAT_BYTES(tp[17], tp[16]);
    dig_p7 = (int16_t)BMX280_CONCAT_BYTES(tp[19], tp[18]);
    dig_p8 = (int16_t)BMX280_CONCAT_BYTES(tp[21], tp[20]);
    dig_p9 = (int16_t)BMX280_CONCAT_BYTES(tp[23], tp[22]);
    if (humidity) {
      uint8_t h[BMX280_LEN_HUMIDITY_CALIB];
      ok = interface.sendAndRead(BMX280_REG_HUMIDITY_CALIB, h, BMX280_LEN_HUMIDITY_CALIB);
      if (ok) {
        dig_h1 = tp[25];
        dig_h2 = (int16_t)BMX280_CONCAT_BYTES(h[1], h[0]);
        dig_h3 = h[2];
        dig_h4 = (int16_t)((int16_t)(int8_t)h[3] * 16) | (int16_t)(h[4] & 0x0F);
        dig_h5 = (int16_t)((int16_t)(int8_t)h[5] * 16) | (int16_t)(h[4] >> 4);
        dig_h6 = (int8_t)h[6];
      }
    }
    #ifdef SENSOR_BMX280_DEBUG
      Serial.print(id); Serial.print(F(" calib t1=")); Serial.print(dig_t1);
      Serial.print(F(" p1=")); Serial.print(dig_p1);
      Serial.print(F(" h1=")); Serial.print(dig_h1);
      Serial.print(F(" h4=")); Serial.print(dig_h4);
      Serial.print(F(" h5=")); Serial.println(dig_h5);
    #endif
  }
  return ok;
}

void Sensor_BMx280::setup() {
  Sensor::setup(); // Will readConfigFromFS - do before touching the device
  interface.initialize();
  delay(10);
  interface.sendRegister(BMX280_REG_RESET, BMX280_CMD_RESET);
  delay(10); // Datasheet startup time is 2ms; be generous

  uint8_t chip_id = 0;
  interface.sendAndRead(BMX280_REG_CHIP_ID, &chip_id, 1);
  #ifdef SENSOR_BMX280_DEBUG
    Serial.print(id); Serial.print(F(" chip id=0x")); Serial.println(chip_id, HEX);
  #endif
  if (!chipIdValid(chip_id)) {
    // 0x58 is a BMP280 (no humidity), 0x60 a BME280, 0x61 a BME680/BME688
    Serial.print(id); Serial.print(F(": wrong chip id 0x")); Serial.print(chip_id, HEX);
    Serial.print(F(" (expected 0x")); Serial.print(chipId, HEX);
    Serial.println(F(") - check address, wiring, and which chip the sketch asks for"));
    setupFailed();
  } else if (!readCalibration()) {
    Serial.print(id); Serial.println(F(": could not read calibration"));
    setupFailed();
  } else {
    if (humidity) {
      // ctrl_hum only takes effect on the next ctrl_meas write, which readValidateConvertSet does
      interface.sendRegister(BMX280_REG_CTRL_HUM, BMX280_CTRL_HUM_X1);
    }
    interface.sendRegister(BMX280_REG_CONFIG, BMX280_CONFIG_FILTER_OFF);
    present = true;
    connected = true;
  }
}

// Ported from Bosch bme280.c compensate_temperature() - BME280_DOUBLE_ENABLE variant
double Sensor_BMx280::compensateTemperature(int32_t raw) {
  double var1 = ((double)raw) / 16384.0 - ((double)dig_t1) / 1024.0;
  var1 = var1 * ((double)dig_t2);
  double var2 = ((double)raw) / 131072.0 - ((double)dig_t1) / 8192.0;
  var2 = (var2 * var2) * ((double)dig_t3);
  t_fine = (int32_t)(var1 + var2); // Needed by pressure and humidity
  double temp = (var1 + var2) / 5120.0; // Local, not the `temperature` output
  if (temp < -40.0) {
    temp = -40.0;
  } else if (temp > 85.0) {
    temp = 85.0;
  }
  return temp;
}

// Ported from Bosch bme280.c compensate_pressure() - returns Pa
double Sensor_BMx280::compensatePressure(int32_t raw) {
  double press = 30000.0; // Bosch's pressure_min, also its invalid-case result
  double var1 = ((double)t_fine / 2.0) - 64000.0;
  double var2 = var1 * var1 * ((double)dig_p6) / 32768.0;
  var2 = var2 + var1 * ((double)dig_p5) * 2.0;
  var2 = (var2 / 4.0) + (((double)dig_p4) * 65536.0);
  double var3 = ((double)dig_p3) * var1 * var1 / 524288.0;
  var1 = (var3 + ((double)dig_p2) * var1) / 524288.0;
  var1 = (1.0 + var1 / 32768.0) * ((double)dig_p1);
  if (var1 > 0.0) { // Avoid division by zero
    press = 1048576.0 - (double)raw;
    press = (press - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = ((double)dig_p9) * press * press / 2147483648.0;
    var2 = press * ((double)dig_p8) / 32768.0;
    press = press + (var1 + var2 + ((double)dig_p7)) / 16.0;
    if (press < 30000.0) {
      press = 30000.0;
    } else if (press > 110000.0) {
      press = 110000.0;
    }
  }
  return press;
}

// Ported from Bosch bme280.c compensate_humidity() - BME280 only
double Sensor_BMx280::compensateHumidity(int32_t raw) {
  double var1 = ((double)t_fine) - 76800.0;
  double var2 = (((double)dig_h4) * 64.0 + (((double)dig_h5) / 16384.0) * var1);
  double var3 = raw - var2;
  double var4 = ((double)dig_h2) / 65536.0;
  double var5 = 1.0 + (((double)dig_h3) / 67108864.0) * var1;
  double var6 = 1.0 + (((double)dig_h6) / 67108864.0) * var1 * var5;
  var6 = var3 * var4 * (var5 * var6);
  double humy = var6 * (1.0 - ((double)dig_h1) * var6 / 524288.0);
  if (humy > 100.0) {
    humy = 100.0;
  } else if (humy < 0.0) {
    humy = 0.0;
  }
  return humy;
}

// Bosch's compensation clamps to the sensor's rated range, so out-of-range raw data would
// silently publish a clamped value (e.g. 85C). Reject the extremes rather than publish them.
// humy arrives as NAN on a BMP280, where there is nothing to check.
bool Sensor_BMx280::validate(float temp, float press, float humy) {
  return !std::isnan(temp) && !std::isnan(press)
      && (temp > -40.0f) && (temp < 85.0f)
      && (press > 300.0f) && (press < 1100.0f)
      && (!humidity || (!std::isnan(humy) && (humy >= 0.0f) && (humy <= 100.0f)));
}

void Sensor_BMx280::readValidateConvertSet() {
  if (present) {
    // Trigger one measurement; the chip returns to sleep on its own afterwards
    interface.sendRegister(BMX280_REG_CTRL_MEAS, BMX280_CTRL_MEAS_FORCED_X1);
    uint32_t start = millis(); // Not sleepSafeMillis as this is a sub-second wait
    uint8_t status = BMX280_STATUS_MEASURING;
    while ((status & BMX280_STATUS_MEASURING) && ((millis() - start) < BMX280_MEASURE_TIMEOUT_MS)) {
      interface.sendAndRead(BMX280_REG_STATUS, &status, 1);
    }
    const uint8_t len = humidity ? BMX280_LEN_DATA : BMX280_LEN_DATA_NO_HUMIDITY;
    uint8_t d[BMX280_LEN_DATA];
    if (interface.sendAndRead(BMX280_REG_DATA, d, len)) {
      // A missing device reads back all 0xFF, which would compensate to clamped extremes
      bool allff = true;
      for (uint8_t i = 0; i < len; i++) {
        if (d[i] != 0xFF) {
          allff = false;
        }
      }
      if (allff) {
        connected = false;
        #ifdef SENSOR_BMX280_DEBUG
          Serial.print(id); Serial.println(F(": all 0xFF - device not responding"));
        #endif
      } else {
        int32_t raw_p = ((uint32_t)d[0] << 12) | ((uint32_t)d[1] << 4) | ((uint32_t)d[2] >> 4);
        int32_t raw_t = ((uint32_t)d[3] << 12) | ((uint32_t)d[4] << 4) | ((uint32_t)d[5] >> 4);
        float temp  = (float)compensateTemperature(raw_t); // Must be first - sets t_fine
        float press = (float)(compensatePressure(raw_p) / 100.0); // Pa -> hPa
        float humy  = NAN;
        if (humidity) {
          humy = (float)compensateHumidity(((uint32_t)d[6] << 8) | (uint32_t)d[7]);
        }
        #ifdef SENSOR_BMX280_DEBUG
          Serial.print(id); Serial.print(F(" ")); Serial.print(temp, 1); Serial.print(F("C "));
          if (humidity) { Serial.print(humy, 1); Serial.print(F("% ")); }
          Serial.print(press, 1); Serial.println(F("hPa"));
        #endif
        if (validate(temp, press, humy)) {
          connected = true;
          temperature->set(temp);
          pressure->set(press);
          if (humidity) {
            humidity->set(humy);
          }
        #ifdef SENSOR_BMX280_DEBUG
        } else {
          Serial.print(id); Serial.println(F(": reading failed validation"));
        #endif
        }
      }
    }
  }
}

Sensor_BMP280::Sensor_BMP280(const char * const name, uint8_t address, TwoWire* wire, bool retain)
  : Sensor_BMx280("bmp280", name, BMP280_CHIP_ID, address, wire, retain, false,
      {DEFAULT_bmp280_temperature_min, DEFAULT_bmp280_temperature_max, DEFAULT_bmp280_temperature_color},
      {DEFAULT_bmp280_pressure_min, DEFAULT_bmp280_pressure_max, DEFAULT_bmp280_pressure_color},
      {0, 0, nullptr}) // No humidity on this chip
  { }

// Bosch's own driver accepts three ids for this chip - 0x58 is what production parts report,
// 0x56 and 0x57 were engineering samples that are still out there on cheap modules.
bool Sensor_BMP280::chipIdValid(uint8_t readId) {
  return (readId == BMP280_CHIP_ID) || (readId == BMP280_CHIP_ID_SAMPLE1) || (readId == BMP280_CHIP_ID_SAMPLE2);
}

Sensor_BME280::Sensor_BME280(const char * const name, uint8_t address, TwoWire* wire, bool retain)
  : Sensor_BMx280("bme280", name, BME280_CHIP_ID, address, wire, retain, true,
      {DEFAULT_bme280_temperature_min, DEFAULT_bme280_temperature_max, DEFAULT_bme280_temperature_color},
      {DEFAULT_bme280_pressure_min, DEFAULT_bme280_pressure_max, DEFAULT_bme280_pressure_color},
      {DEFAULT_bme280_humidity_min, DEFAULT_bme280_humidity_max, DEFAULT_bme280_humidity_color})
  { }
