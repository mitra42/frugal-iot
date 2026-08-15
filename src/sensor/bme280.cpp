/* Frugal IoT - BME280 temperature, humidity and pressure sensor
 *
 * NOTE - AS OF 2026-08-03 THIS IS UNTESTED CODE
 *
 * See sensor/bme280.h for the licence attribution, register configuration and rationale.
 *
 * Compensation and calibration unpacking ported from Bosch's reference driver:
 *   https://github.com/boschsensortec/BME280_SensorAPI  (bme280.c, BME280_DOUBLE_ENABLE)
 *   Copyright (c) 2023 Bosch Sensortec GmbH. SPDX-License-Identifier: BSD-3-Clause
 */

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "sensor/bme280.h"
#include "cmath" // for std::isnan

// Register map, per the Bosch reference driver
#define BME280_REG_CHIP_ID              0xD0
#define BME280_REG_RESET                0xE0
#define BME280_REG_TEMP_PRESS_CALIB     0x88
#define BME280_REG_HUMIDITY_CALIB       0xE1
#define BME280_REG_CTRL_HUM             0xF2
#define BME280_REG_STATUS               0xF3
#define BME280_REG_CTRL_MEAS            0xF4
#define BME280_REG_CONFIG               0xF5
#define BME280_REG_DATA                 0xF7

#define BME280_CHIP_ID                  0x60 // A BMP280 answers 0x58 and has no humidity
#define BME280_CMD_RESET                0xB6
#define BME280_LEN_TEMP_PRESS_CALIB     26
#define BME280_LEN_HUMIDITY_CALIB       7
#define BME280_LEN_DATA                 8
#define BME280_STATUS_MEASURING         0x08

// Bosch "weather monitoring": forced mode, 1x oversampling everywhere, IIR filter off.
#define BME280_CTRL_HUM_X1              0x01 // osrs_h = 001
#define BME280_CTRL_MEAS_FORCED_X1      0x25 // osrs_t = 001, osrs_p = 001, mode = 01 (forced)
#define BME280_CONFIG_FILTER_OFF        0x00
#define BME280_MEASURE_TIMEOUT_MS       50   // Max measurement time at 1x is ~8ms

#define BME280_CONCAT_BYTES(msb, lsb)   (((uint16_t)msb << 8) | (uint16_t)lsb)

Sensor_BME280::Sensor_BME280(const char * const name, uint8_t address, TwoWire* wire, bool retain)
  : Sensor_HT("bme280", name, retain),
    interface(address, wire),
    wire(wire)
{
  //TODO-213 define min/max/color in the UX defaults (generate-defaults.js) - literals for now, as bh1750 does
  outputs.push_back(pressure = new OUTfloat("bme280", "pressure", "Pressure", 0, 1, 300, 1100, "blue", false));
}

// 26 bytes from 0x88 (dig_t1..dig_p9 then dig_h1), then 7 bytes from 0xE1 (dig_h2..dig_h6).
// dig_h4 and dig_h5 share the nibbles of the byte at 0xE5 - note the sign extension of the
// MSB happens before the shift, which is what makes this worth porting rather than guessing.
bool Sensor_BME280::readCalibration() {
  uint8_t tp[BME280_LEN_TEMP_PRESS_CALIB];
  uint8_t h[BME280_LEN_HUMIDITY_CALIB];
  bool ok = interface.sendAndRead(BME280_REG_TEMP_PRESS_CALIB, tp, BME280_LEN_TEMP_PRESS_CALIB)
         && interface.sendAndRead(BME280_REG_HUMIDITY_CALIB, h, BME280_LEN_HUMIDITY_CALIB);
  if (ok) {
    dig_t1 = BME280_CONCAT_BYTES(tp[1], tp[0]);
    dig_t2 = (int16_t)BME280_CONCAT_BYTES(tp[3], tp[2]);
    dig_t3 = (int16_t)BME280_CONCAT_BYTES(tp[5], tp[4]);
    dig_p1 = BME280_CONCAT_BYTES(tp[7], tp[6]);
    dig_p2 = (int16_t)BME280_CONCAT_BYTES(tp[9], tp[8]);
    dig_p3 = (int16_t)BME280_CONCAT_BYTES(tp[11], tp[10]);
    dig_p4 = (int16_t)BME280_CONCAT_BYTES(tp[13], tp[12]);
    dig_p5 = (int16_t)BME280_CONCAT_BYTES(tp[15], tp[14]);
    dig_p6 = (int16_t)BME280_CONCAT_BYTES(tp[17], tp[16]);
    dig_p7 = (int16_t)BME280_CONCAT_BYTES(tp[19], tp[18]);
    dig_p8 = (int16_t)BME280_CONCAT_BYTES(tp[21], tp[20]);
    dig_p9 = (int16_t)BME280_CONCAT_BYTES(tp[23], tp[22]);
    dig_h1 = tp[25];
    dig_h2 = (int16_t)BME280_CONCAT_BYTES(h[1], h[0]);
    dig_h3 = h[2];
    dig_h4 = (int16_t)((int16_t)(int8_t)h[3] * 16) | (int16_t)(h[4] & 0x0F);
    dig_h5 = (int16_t)((int16_t)(int8_t)h[5] * 16) | (int16_t)(h[4] >> 4);
    dig_h6 = (int8_t)h[6];
    #ifdef SENSOR_BME280_DEBUG
      Serial.print(F("BME280 calib t1=")); Serial.print(dig_t1);
      Serial.print(F(" p1=")); Serial.print(dig_p1);
      Serial.print(F(" h1=")); Serial.print(dig_h1);
      Serial.print(F(" h4=")); Serial.print(dig_h4);
      Serial.print(F(" h5=")); Serial.println(dig_h5);
    #endif
  }
  return ok;
}

void Sensor_BME280::setup() {
  Sensor_HT::setup(); // Will readConfigFromFS - do before touching the device
  interface.initialize();
  delay(10);
  interface.sendRegister(BME280_REG_RESET, BME280_CMD_RESET);
  delay(10); // Datasheet startup time is 2ms; be generous

  uint8_t chip_id = 0;
  interface.sendAndRead(BME280_REG_CHIP_ID, &chip_id, 1);
  #ifdef SENSOR_BME280_DEBUG
    Serial.print(F("BME280 chip id=0x")); Serial.println(chip_id, HEX);
  #endif
  if (chip_id != BME280_CHIP_ID) {
    // 0x58 is a BMP280 - no humidity, so it cannot be used through Sensor_HT
    Serial.print(F("BME280: wrong chip id 0x")); Serial.print(chip_id, HEX);
    Serial.println(F(" (expected 0x60) - check address and wiring"));
    setupFailed();
  } else if (!readCalibration()) {
    Serial.println(F("BME280: could not read calibration"));
    setupFailed();
  } else {
    // ctrl_hum only takes effect on the next ctrl_meas write, which readValidateConvertSet does
    interface.sendRegister(BME280_REG_CTRL_HUM, BME280_CTRL_HUM_X1);
    interface.sendRegister(BME280_REG_CONFIG, BME280_CONFIG_FILTER_OFF);
    present = true;
    connected = true;
  }
}

// Ported from Bosch bme280.c compensate_temperature() - BME280_DOUBLE_ENABLE variant
double Sensor_BME280::compensateTemperature(int32_t raw) {
  double var1 = ((double)raw) / 16384.0 - ((double)dig_t1) / 1024.0;
  var1 = var1 * ((double)dig_t2);
  double var2 = ((double)raw) / 131072.0 - ((double)dig_t1) / 8192.0;
  var2 = (var2 * var2) * ((double)dig_t3);
  t_fine = (int32_t)(var1 + var2); // Needed by pressure and humidity
  double temperature = (var1 + var2) / 5120.0;
  if (temperature < -40.0) {
    temperature = -40.0;
  } else if (temperature > 85.0) {
    temperature = 85.0;
  }
  return temperature;
}

// Ported from Bosch bme280.c compensate_pressure() - returns Pa
double Sensor_BME280::compensatePressure(int32_t raw) {
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

// Ported from Bosch bme280.c compensate_humidity()
double Sensor_BME280::compensateHumidity(int32_t raw) {
  double var1 = ((double)t_fine) - 76800.0;
  double var2 = (((double)dig_h4) * 64.0 + (((double)dig_h5) / 16384.0) * var1);
  double var3 = raw - var2;
  double var4 = ((double)dig_h2) / 65536.0;
  double var5 = 1.0 + (((double)dig_h3) / 67108864.0) * var1;
  double var6 = 1.0 + (((double)dig_h6) / 67108864.0) * var1 * var5;
  var6 = var3 * var4 * (var5 * var6);
  double humidity = var6 * (1.0 - ((double)dig_h1) * var6 / 524288.0);
  if (humidity > 100.0) {
    humidity = 100.0;
  } else if (humidity < 0.0) {
    humidity = 0.0;
  }
  return humidity;
}

// Bosch's compensation clamps to the sensor's rated range, so out-of-range raw data would
// silently publish a clamped value (e.g. 85C). Reject the extremes rather than publish them.
bool Sensor_BME280::validate(float temp, float humy, float press) {
  return !std::isnan(temp) && !std::isnan(humy) && !std::isnan(press)
      && (temp > -40.0f) && (temp < 85.0f)
      && (humy >= 0.0f) && (humy <= 100.0f)
      && (press > 300.0f) && (press < 1100.0f);
}

void Sensor_BME280::readValidateConvertSet() {
  if (present) {
    // Trigger one measurement; the chip returns to sleep on its own afterwards
    interface.sendRegister(BME280_REG_CTRL_MEAS, BME280_CTRL_MEAS_FORCED_X1);
    uint32_t start = millis(); // Not sleepSafeMillis as this is a sub-second wait
    uint8_t status = BME280_STATUS_MEASURING;
    while ((status & BME280_STATUS_MEASURING) && ((millis() - start) < BME280_MEASURE_TIMEOUT_MS)) {
      interface.sendAndRead(BME280_REG_STATUS, &status, 1);
    }
    uint8_t d[BME280_LEN_DATA];
    if (interface.sendAndRead(BME280_REG_DATA, d, BME280_LEN_DATA)) {
      // A missing device reads back all 0xFF, which would compensate to clamped extremes
      bool allff = true;
      for (uint8_t i = 0; i < BME280_LEN_DATA; i++) {
        if (d[i] != 0xFF) {
          allff = false;
        }
      }
      if (allff) {
        connected = false;
        #ifdef SENSOR_BME280_DEBUG
          Serial.println(F("BME280: all 0xFF - device not responding"));
        #endif
      } else {
        int32_t raw_p = ((uint32_t)d[0] << 12) | ((uint32_t)d[1] << 4) | ((uint32_t)d[2] >> 4);
        int32_t raw_t = ((uint32_t)d[3] << 12) | ((uint32_t)d[4] << 4) | ((uint32_t)d[5] >> 4);
        int32_t raw_h = ((uint32_t)d[6] << 8)  | (uint32_t)d[7];
        float temp  = (float)compensateTemperature(raw_t); // Must be first - sets t_fine
        float press = (float)(compensatePressure(raw_p) / 100.0); // Pa -> hPa
        float humy  = (float)compensateHumidity(raw_h);
        #ifdef SENSOR_BME280_DEBUG
          Serial.print(F("BME280 ")); Serial.print(temp, 1); Serial.print(F("C "));
          Serial.print(humy, 1); Serial.print(F("% "));
          Serial.print(press, 1); Serial.println(F("hPa"));
        #endif
        if (validate(temp, humy, press)) {
          connected = true;
          set(temp, humy);          // Sensor_HT sets temperature and humidity
          pressure->set(press);
        #ifdef SENSOR_BME280_DEBUG
        } else {
          Serial.println(F("BME280: reading failed validation"));
        #endif
        }
      }
    }
  }
}

void Sensor_BME280::captiveLines(AsyncResponseStream* response) {
  // Sensor_HT::captiveLines only knows about temperature and humidity
  response->print(String(F("<p><label>")) + name
    + "<br>Temperature: " + temperature->StringValue() + " C"
    + "<br>Humidity: " + humidity->StringValue() + " %"
    + "<br>Pressure: " + pressure->StringValue() + " hPa</label></p>");
}
