/* Frugal IoT - BME680 temperature, humidity, pressure and gas sensor
 *
 * NOTE - AS OF 2026-08-11 THIS IS UNTESTED CODE
 *
 * See sensor/bme680.h for the licence attribution, register configuration and rationale.
 *
 * Compensation, calibration unpacking and heater arithmetic ported from Bosch's reference driver:
 *   https://github.com/boschsensortec/BME68x-Sensor-API  (bme68x.c, BME68X_USE_FPU)
 *   Copyright (c) 2023 Bosch Sensortec GmbH. SPDX-License-Identifier: BSD-3-Clause
 */

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "sensor/bme680.h"
#include "cmath" // for std::isnan

// Register map, per the Bosch reference driver
#define BME680_REG_COEFF3               0x00
#define BME680_REG_FIELD0               0x1D
#define BME680_REG_RES_HEAT0            0x5A
#define BME680_REG_GAS_WAIT0            0x64
#define BME680_REG_CTRL_GAS_0           0x70
#define BME680_REG_CTRL_GAS_1           0x71
#define BME680_REG_CTRL_HUM             0x72
#define BME680_REG_CTRL_MEAS            0x74
#define BME680_REG_CONFIG               0x75
#define BME680_REG_COEFF1               0x8A
#define BME680_REG_CHIP_ID              0xD0
#define BME680_REG_RESET                0xE0
#define BME680_REG_COEFF2               0xE1
#define BME680_REG_VARIANT_ID           0xF0

#define BME680_CHIP_ID                  0x61 // Both BME680 and BME688 - the variant id separates them
#define BME680_VARIANT_GAS_LOW          0x00 // BME680
#define BME680_VARIANT_GAS_HIGH         0x01 // BME688
#define BME680_CMD_RESET                0xB6

// The calibration coefficients arrive as three separate register blocks, which the reference
// driver concatenates into one 42-byte array and indexes with the constants below. Kept in
// exactly that layout so the unpacking can be read against Bosch's get_calib_data().
#define BME680_LEN_COEFF1               23 // From 0x8A
#define BME680_LEN_COEFF2               14 // From 0xE1
#define BME680_LEN_COEFF3               5  // From 0x00
#define BME680_LEN_COEFF_ALL            42
#define BME680_LEN_FIELD                17 // From 0x1D

#define BME680_IDX_T2_LSB               0
#define BME680_IDX_T2_MSB               1
#define BME680_IDX_T3                   2
#define BME680_IDX_P1_LSB               4
#define BME680_IDX_P1_MSB               5
#define BME680_IDX_P2_LSB               6
#define BME680_IDX_P2_MSB               7
#define BME680_IDX_P3                   8
#define BME680_IDX_P4_LSB               10
#define BME680_IDX_P4_MSB               11
#define BME680_IDX_P5_LSB               12
#define BME680_IDX_P5_MSB               13
#define BME680_IDX_P7                   14 // Note P7 before P6, as on the chip
#define BME680_IDX_P6                   15
#define BME680_IDX_P8_LSB               18
#define BME680_IDX_P8_MSB               19
#define BME680_IDX_P9_LSB               20
#define BME680_IDX_P9_MSB               21
#define BME680_IDX_P10                  22
#define BME680_IDX_H2_MSB               23
#define BME680_IDX_H2_LSB               24 // H1 and H2 share the nibbles of this byte
#define BME680_IDX_H1_LSB               24
#define BME680_IDX_H1_MSB               25
#define BME680_IDX_H3                   26
#define BME680_IDX_H4                   27
#define BME680_IDX_H5                   28
#define BME680_IDX_H6                   29
#define BME680_IDX_H7                   30
#define BME680_IDX_T1_LSB               31
#define BME680_IDX_T1_MSB               32
#define BME680_IDX_GH2_LSB              33
#define BME680_IDX_GH2_MSB              34
#define BME680_IDX_GH1                  35
#define BME680_IDX_GH3                  36
#define BME680_IDX_RES_HEAT_VAL         37
#define BME680_IDX_RES_HEAT_RANGE       39
#define BME680_IDX_RANGE_SW_ERR         41

// Bosch "weather monitoring": forced mode, 1x oversampling everywhere, IIR filter off.
#define BME680_CTRL_HUM_X1              0x01 // osrs_h = 001
#define BME680_CTRL_MEAS_FORCED_X1      0x25 // osrs_t = 001, osrs_p = 001, mode = 01 (forced)
#define BME680_CTRL_MEAS_SLEEP_X1       0x24 // Same, mode = 00 - registers only change in sleep
#define BME680_CONFIG_FILTER_OFF        0x00
#define BME680_CTRL_GAS_0_HEATER_ON     0x00 // heat_off = 0
#define BME680_CTRL_GAS_0_HEATER_OFF    0x08 // heat_off = 1
#define BME680_CTRL_GAS_1_RUN_LOW       0x10 // run_gas = 01, nb_conv = 0 (heater profile 0)
#define BME680_CTRL_GAS_1_RUN_HIGH      0x20 // run_gas = 10 on the BME688
#define BME680_CTRL_GAS_1_OFF           0x00

#define BME680_NEW_DATA_MSK             0x80
#define BME680_GASM_VALID_MSK           0x20
#define BME680_HEAT_STAB_MSK            0x10
#define BME680_GAS_RANGE_MSK            0x0F
#define BME680_RHRANGE_MSK              0x30
#define BME680_RSERROR_MSK              0xF0
#define BME680_BIT_H1_DATA_MSK          0x0F

// Bosch's bme68x_get_meas_dur() for 1x on all three channels: (1+1+1)*1963us of conversion,
// 477*4 switching, 477*5 gas, 1000 wake-up = 11182us. The heater time is on top of this.
#define BME680_TPH_DURATION_MS          12
#define BME680_POLL_INTERVAL_MS         5

#define BME680_CONCAT_BYTES(msb, lsb)   (((uint16_t)msb << 8) | (uint16_t)lsb)

Sensor_BME680::Sensor_BME680(const char * const name, uint8_t address, TwoWire* wire, bool retain, bool wantGas)
  : Sensor("bme680", name, retain),
    temperature(new OUTfloat("bme680", "temperature", "Temperature", 0, 1, DEFAULT_bme680_temperature_min, DEFAULT_bme680_temperature_max, DEFAULT_bme680_temperature_color, false)),
    humidity(new OUTfloat("bme680", "humidity", "Humidity", 0, 1, DEFAULT_bme680_humidity_min, DEFAULT_bme680_humidity_max, DEFAULT_bme680_humidity_color, false)),
    pressure(new OUTfloat("bme680", "pressure", "Pressure", 0, 1, DEFAULT_bme680_pressure_min, DEFAULT_bme680_pressure_max, DEFAULT_bme680_pressure_color, false)),
    gas(nullptr), // Stays null if wantGas is false - the presence of the output is the flag
    interface(address, wire),
    wire(wire)
{
  outputs.push_back(temperature);
  outputs.push_back(humidity);
  outputs.push_back(pressure);
  temperature->unit = "C";
  humidity->unit = "%";
  pressure->unit = "hPa";
  if (wantGas) {
    // Practical range is a few kOhm in dirty air to a few hundred in clean air, but it is a
    // relative measure - see the note on IAQ in bme680.h.
    outputs.push_back(gas = new OUTfloat("bme680", "gas", "Gas Resistance", 0, 1, DEFAULT_bme680_gas_min, DEFAULT_bme680_gas_max, DEFAULT_bme680_gas_color, false));
    gas->unit = "kOhm";
  }
}

// 23 bytes from 0x8A, 14 from 0xE1 and 5 from 0x00, concatenated into the one array the
// reference driver indexes. Note par_h1 and par_h2 share the nibbles of the byte at 0xE2, and
// that par_p7 sits *before* par_p6 in the register order.
bool Sensor_BME680::readCalibration() {
  uint8_t c[BME680_LEN_COEFF_ALL];
  bool ok = interface.sendAndRead(BME680_REG_COEFF1, c, BME680_LEN_COEFF1)
         && interface.sendAndRead(BME680_REG_COEFF2, &c[BME680_LEN_COEFF1], BME680_LEN_COEFF2)
         && interface.sendAndRead(BME680_REG_COEFF3, &c[BME680_LEN_COEFF1 + BME680_LEN_COEFF2], BME680_LEN_COEFF3);
  if (ok) {
    par_t1 = BME680_CONCAT_BYTES(c[BME680_IDX_T1_MSB], c[BME680_IDX_T1_LSB]);
    par_t2 = (int16_t)BME680_CONCAT_BYTES(c[BME680_IDX_T2_MSB], c[BME680_IDX_T2_LSB]);
    par_t3 = (int8_t)c[BME680_IDX_T3];
    par_p1 = BME680_CONCAT_BYTES(c[BME680_IDX_P1_MSB], c[BME680_IDX_P1_LSB]);
    par_p2 = (int16_t)BME680_CONCAT_BYTES(c[BME680_IDX_P2_MSB], c[BME680_IDX_P2_LSB]);
    par_p3 = (int8_t)c[BME680_IDX_P3];
    par_p4 = (int16_t)BME680_CONCAT_BYTES(c[BME680_IDX_P4_MSB], c[BME680_IDX_P4_LSB]);
    par_p5 = (int16_t)BME680_CONCAT_BYTES(c[BME680_IDX_P5_MSB], c[BME680_IDX_P5_LSB]);
    par_p6 = (int8_t)c[BME680_IDX_P6];
    par_p7 = (int8_t)c[BME680_IDX_P7];
    par_p8 = (int16_t)BME680_CONCAT_BYTES(c[BME680_IDX_P8_MSB], c[BME680_IDX_P8_LSB]);
    par_p9 = (int16_t)BME680_CONCAT_BYTES(c[BME680_IDX_P9_MSB], c[BME680_IDX_P9_LSB]);
    par_p10 = c[BME680_IDX_P10];
    par_h1 = (uint16_t)(((uint16_t)c[BME680_IDX_H1_MSB] << 4) | (c[BME680_IDX_H1_LSB] & BME680_BIT_H1_DATA_MSK));
    par_h2 = (uint16_t)(((uint16_t)c[BME680_IDX_H2_MSB] << 4) | (c[BME680_IDX_H2_LSB] >> 4));
    par_h3 = (int8_t)c[BME680_IDX_H3];
    par_h4 = (int8_t)c[BME680_IDX_H4];
    par_h5 = (int8_t)c[BME680_IDX_H5];
    par_h6 = c[BME680_IDX_H6];
    par_h7 = (int8_t)c[BME680_IDX_H7];
    par_gh1 = (int8_t)c[BME680_IDX_GH1];
    par_gh2 = (int16_t)BME680_CONCAT_BYTES(c[BME680_IDX_GH2_MSB], c[BME680_IDX_GH2_LSB]);
    par_gh3 = (int8_t)c[BME680_IDX_GH3];
    res_heat_range = (c[BME680_IDX_RES_HEAT_RANGE] & BME680_RHRANGE_MSK) / 16;
    res_heat_val = (int8_t)c[BME680_IDX_RES_HEAT_VAL];
    range_sw_err = ((int8_t)(c[BME680_IDX_RANGE_SW_ERR] & BME680_RSERROR_MSK)) / 16;
    #ifdef SENSOR_BME680_DEBUG
      Serial.print(F("BME680 calib t1=")); Serial.print(par_t1);
      Serial.print(F(" p1=")); Serial.print(par_p1);
      Serial.print(F(" h1=")); Serial.print(par_h1);
      Serial.print(F(" h2=")); Serial.print(par_h2);
      Serial.print(F(" gh1=")); Serial.print(par_gh1);
      Serial.print(F(" heat_range=")); Serial.print(res_heat_range);
      Serial.print(F(" heat_val=")); Serial.print(res_heat_val);
      Serial.print(F(" sw_err=")); Serial.println(range_sw_err);
    #endif
  }
  return ok;
}

void Sensor_BME680::setup() {
  Sensor::setup(); // Will readConfigFromFS - do before touching the device
  interface.initialize();
  delay(10);
  interface.sendRegister(BME680_REG_RESET, BME680_CMD_RESET);
  delay(10); // Datasheet startup time is 2ms; be generous

  uint8_t chip_id = 0;
  interface.sendAndRead(BME680_REG_CHIP_ID, &chip_id, 1);
  #ifdef SENSOR_BME680_DEBUG
    Serial.print(F("BME680 chip id=0x")); Serial.println(chip_id, HEX);
  #endif
  if (chip_id != BME680_CHIP_ID) {
    // 0x60 is a BME280, 0x58 a BMP280 - similar boards, different register maps
    Serial.print(F("BME680: wrong chip id 0x")); Serial.print(chip_id, HEX);
    Serial.println(F(" (expected 0x61) - check address and wiring"));
    setupFailed();
  } else if (!readCalibration()) {
    Serial.println(F("BME680: could not read calibration"));
    setupFailed();
  } else {
    // 0x00 = BME680, 0x01 = BME688 - picks the gas formula and the run_gas bit pattern
    interface.sendAndRead(BME680_REG_VARIANT_ID, &variant, 1);
    #ifdef SENSOR_BME680_DEBUG
      Serial.print(F("BME680 variant=0x")); Serial.print(variant, HEX);
      Serial.println(variant == BME680_VARIANT_GAS_HIGH ? F(" (BME688)") : F(" (BME680)"));
    #endif
    present = true;
    connected = true;
  }
}

// Every register the measurement depends on, rewritten before each read. Cheaper than being
// wrong: this class is usable on a node that cuts power to its sensors between cycles, and a
// chip that has been reset silently reverts to sleep-mode defaults with the heater off - which
// would publish temperature and humidity quite happily and nothing but a stuck gas reading.
// The chip is in sleep here (forced mode returns to sleep on its own), which is the only state
// in which these take effect; ctrl_hum in particular only latches when ctrl_meas is written.
void Sensor_BME680::configureMeasurement() {
  if (gas) {
    interface.sendRegister(BME680_REG_RES_HEAT0, calcResHeat(SENSOR_BME680_HEATER_TEMP_C));
    interface.sendRegister(BME680_REG_GAS_WAIT0, calcGasWait(SENSOR_BME680_HEATER_MS));
    interface.sendRegister(BME680_REG_CTRL_GAS_0, BME680_CTRL_GAS_0_HEATER_ON);
    interface.sendRegister(BME680_REG_CTRL_GAS_1,
      (variant == BME680_VARIANT_GAS_HIGH) ? BME680_CTRL_GAS_1_RUN_HIGH : BME680_CTRL_GAS_1_RUN_LOW);
  } else {
    interface.sendRegister(BME680_REG_CTRL_GAS_0, BME680_CTRL_GAS_0_HEATER_OFF);
    interface.sendRegister(BME680_REG_CTRL_GAS_1, BME680_CTRL_GAS_1_OFF);
  }
  interface.sendRegister(BME680_REG_CTRL_HUM, BME680_CTRL_HUM_X1);
  interface.sendRegister(BME680_REG_CONFIG, BME680_CONFIG_FILTER_OFF);
  interface.sendRegister(BME680_REG_CTRL_MEAS, BME680_CTRL_MEAS_SLEEP_X1); // Oversampling, still asleep
}

// Ported from Bosch bme68x.c calc_temperature() - BME68X_USE_FPU variant
float Sensor_BME680::compensateTemperature(uint32_t raw) {
  float var1 = (((float)raw / 16384.0f) - ((float)par_t1 / 1024.0f)) * ((float)par_t2);
  float var2 = ((((float)raw / 131072.0f) - ((float)par_t1 / 8192.0f)) *
                (((float)raw / 131072.0f) - ((float)par_t1 / 8192.0f))) * ((float)par_t3 * 16.0f);
  t_fine = var1 + var2; // Needed by pressure and humidity
  return t_fine / 5120.0f;
}

// Ported from Bosch bme68x.c calc_pressure() - returns Pa
float Sensor_BME680::compensatePressure(uint32_t raw) {
  float var1 = ((float)t_fine / 2.0f) - 64000.0f;
  float var2 = var1 * var1 * (((float)par_p6) / 131072.0f);
  var2 = var2 + (var1 * ((float)par_p5) * 2.0f);
  var2 = (var2 / 4.0f) + (((float)par_p4) * 65536.0f);
  var1 = (((((float)par_p3 * var1 * var1) / 16384.0f) + ((float)par_p2 * var1)) / 524288.0f);
  var1 = ((1.0f + (var1 / 32768.0f)) * ((float)par_p1));
  float press = 1048576.0f - (float)raw;
  if ((int)var1 != 0) { // Avoid division by zero
    press = ((press - (var2 / 4096.0f)) * 6250.0f) / var1;
    var1 = (((float)par_p9) * press * press) / 2147483648.0f;
    var2 = press * (((float)par_p8) / 32768.0f);
    float var3 = (press / 256.0f) * (press / 256.0f) * (press / 256.0f) * (par_p10 / 131072.0f);
    press = press + (var1 + var2 + var3 + ((float)par_p7 * 128.0f)) / 16.0f;
  } else {
    press = 0;
  }
  return press;
}

// Ported from Bosch bme68x.c calc_humidity()
float Sensor_BME680::compensateHumidity(uint16_t raw) {
  float temp_comp = t_fine / 5120.0f;
  float var1 = (float)raw - (((float)par_h1 * 16.0f) + (((float)par_h3 / 2.0f) * temp_comp));
  float var2 = var1 * ((float)(((float)par_h2 / 262144.0f) *
    (1.0f + (((float)par_h4 / 16384.0f) * temp_comp) +
     (((float)par_h5 / 1048576.0f) * temp_comp * temp_comp))));
  float var3 = (float)par_h6 / 16384.0f;
  float var4 = (float)par_h7 / 2097152.0f;
  float humidity = var2 + ((var3 + (var4 * temp_comp)) * var2 * var2);
  if (humidity > 100.0f) {
    humidity = 100.0f;
  } else if (humidity < 0.0f) {
    humidity = 0.0f;
  }
  return humidity;
}

// Ported from Bosch bme68x.c calc_gas_resistance_low() - the BME680. Returns ohms.
// The two lookup tables correct for the chip's switched measurement ranges; they are data, not
// something that can be derived, which is the main reason this is ported rather than written.
float Sensor_BME680::compensateGasLow(uint16_t raw, uint8_t range) {
  static const float lookup_k1_range[16] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, -0.8f, 0.0f, 0.0f, -0.2f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f
  };
  static const float lookup_k2_range[16] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.1f, 0.7f, 0.0f, -0.8f, -0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
  };
  float gas_range_f = (float)(1U << range);
  float var1 = 1340.0f + (5.0f * range_sw_err);
  float var2 = var1 * (1.0f + lookup_k1_range[range] / 100.0f);
  float var3 = 1.0f + (lookup_k2_range[range] / 100.0f);
  return 1.0f / (float)(var3 * 0.000000125f * gas_range_f * ((((float)raw - 512.0f) / var2) + 1.0f));
}

// Ported from Bosch bme68x.c calc_gas_resistance_high() - the BME688. Returns ohms.
float Sensor_BME680::compensateGasHigh(uint16_t raw, uint8_t range) {
  uint32_t var1 = (uint32_t)262144 >> range;
  int32_t var2 = (int32_t)raw - (int32_t)512;
  var2 *= 3;
  var2 = 4096 + var2;
  return 1000000.0f * (float)var1 / (float)var2;
}

// Ported from Bosch bme68x.c calc_res_heat(). Turns a target plate temperature into the
// register value that gets it there, using the per-chip heater calibration and the *ambient*
// temperature - which is why this is recomputed each read from the last reading rather than
// written once in setup() against Bosch's assumed 25C.
uint8_t Sensor_BME680::calcResHeat(uint16_t temp_c) {
  if (temp_c > 400) { // Cap temperature
    temp_c = 400;
  }
  float var1 = (((float)par_gh1 / 16.0f) + 49.0f);
  float var2 = ((((float)par_gh2 / 32768.0f) * 0.0005f) + 0.00235f);
  float var3 = ((float)par_gh3 / 1024.0f);
  float var4 = (var1 * (1.0f + (var2 * (float)temp_c)));
  float var5 = (var4 + (var3 * (float)ambient_c));
  return (uint8_t)(3.4f * ((var5 * (4 / (4 + (float)res_heat_range)) *
                            (1 / (1 + ((float)res_heat_val * 0.002f)))) - 25));
}

// Ported from Bosch bme68x.c calc_gas_wait() - milliseconds into the chip's value+multiplier
// encoding: the low 6 bits are a count, the top 2 a x4 multiplier applied that many times.
uint8_t Sensor_BME680::calcGasWait(uint16_t dur_ms) {
  uint8_t durval;
  if (dur_ms >= 0xFC0) {
    durval = 0xFF; // Max duration
  } else {
    uint8_t factor = 0;
    while (dur_ms > 0x3F) {
      dur_ms = dur_ms / 4;
      factor += 1;
    }
    durval = (uint8_t)(dur_ms + (factor * 64));
  }
  return durval;
}

// Unlike the BME280's, Bosch's BME680 compensation does not clamp temperature or pressure to
// the rated range, so a nonsense reading arrives as a nonsense number rather than a suspicious
// 85C. Reject anything outside the datasheet's operating range either way.
bool Sensor_BME680::validate(float temp, float humy, float press) {
  return !std::isnan(temp) && !std::isnan(humy) && !std::isnan(press)
      && (temp > -40.0f) && (temp < 85.0f)
      && (humy >= 0.0f) && (humy <= 100.0f)
      && (press > 300.0f) && (press < 1100.0f);
}

// Gas resistance has no datasheet range to check against - the useful signal is its trend, and
// a stuck heater shows up in the status bits rather than here. So this only rejects arithmetic
// that has gone off the rails: the compensation divides by a term that approaches zero at the
// extremes of the ADC range, giving a huge or negative resistance.
bool Sensor_BME680::validateGas(float kohm) {
  return !std::isnan(kohm) && (kohm > 0.0f) && (kohm < 100000.0f);
}

void Sensor_BME680::readValidateConvertSet() {
  if (present) {
    configureMeasurement();
    // Trigger one measurement; the chip returns to sleep on its own afterwards
    interface.sendRegister(BME680_REG_CTRL_MEAS, BME680_CTRL_MEAS_FORCED_X1);
    // Wait out the conversion before looking at the new-data bit, rather than polling from the
    // start: the bit is only cleared by reading the field, so a previous cycle that failed
    // part-way through would leave it set and we would read that stale field instead.
    delay(BME680_TPH_DURATION_MS + (gas ? SENSOR_BME680_HEATER_MS : 0));
    uint32_t start = millis(); // Not sleepSafeMillis as this is a sub-second wait
    uint8_t status = 0;
    while (!(status & BME680_NEW_DATA_MSK) && ((millis() - start) < SENSOR_BME680_MEASURE_TIMEOUT_MS)) {
      interface.sendAndRead(BME680_REG_FIELD0, &status, 1);
      if (!(status & BME680_NEW_DATA_MSK)) {
        delay(BME680_POLL_INTERVAL_MS);
      }
    }
    uint8_t d[BME680_LEN_FIELD];
    if (!(status & BME680_NEW_DATA_MSK)) {
      connected = false;
      #ifdef SENSOR_BME680_DEBUG
        Serial.println(F("BME680: timed out waiting for a measurement"));
      #endif
    } else if (interface.sendAndRead(BME680_REG_FIELD0, d, BME680_LEN_FIELD)) {
      // A missing device reads back all 0xFF, which is a plausible-looking raw measurement
      bool allff = true;
      for (uint8_t i = 0; i < BME680_LEN_FIELD; i++) {
        if (d[i] != 0xFF) {
          allff = false;
        }
      }
      if (allff) {
        connected = false;
        #ifdef SENSOR_BME680_DEBUG
          Serial.println(F("BME680: all 0xFF - device not responding"));
        #endif
      } else {
        uint32_t raw_p = ((uint32_t)d[2] << 12) | ((uint32_t)d[3] << 4) | ((uint32_t)d[4] >> 4);
        uint32_t raw_t = ((uint32_t)d[5] << 12) | ((uint32_t)d[6] << 4) | ((uint32_t)d[7] >> 4);
        uint16_t raw_h = ((uint16_t)d[8] << 8) | (uint16_t)d[9];
        // The BME688 reports its gas measurement in a second pair of registers, not the first
        bool high = (variant == BME680_VARIANT_GAS_HIGH);
        uint8_t gas_msb = high ? d[15] : d[13];
        uint8_t gas_lsb = high ? d[16] : d[14];
        uint16_t raw_g = ((uint16_t)gas_msb << 2) | ((uint16_t)gas_lsb >> 6);
        uint8_t gas_range = gas_lsb & BME680_GAS_RANGE_MSK;
        float temp  = compensateTemperature(raw_t); // Must be first - sets t_fine
        float press = compensatePressure(raw_p) / 100.0f; // Pa -> hPa
        float humy  = compensateHumidity(raw_h);
        #ifdef SENSOR_BME680_DEBUG
          Serial.print(F("BME680 ")); Serial.print(temp, 1); Serial.print(F("C "));
          Serial.print(humy, 1); Serial.print(F("% "));
          Serial.print(press, 1); Serial.print(F("hPa status=0x")); Serial.println(gas_lsb, HEX);
        #endif
        if (validate(temp, humy, press)) {
          connected = true;
          temperature->set(temp);
          humidity->set(humy);
          pressure->set(press);
          ambient_c = (int8_t)temp; // Feeds the next cycle's heater resistance
          if (gas) {
            // gas_valid says the plate was actually measured, heat_stab that it had reached
            // its target - the first reading after power-up typically has neither
            if ((gas_lsb & BME680_GASM_VALID_MSK) && (gas_lsb & BME680_HEAT_STAB_MSK)) {
              float kohm = (high ? compensateGasHigh(raw_g, gas_range)
                                 : compensateGasLow(raw_g, gas_range)) / 1000.0f;
              #ifdef SENSOR_BME680_DEBUG
                Serial.print(F("BME680 gas ")); Serial.print(kohm, 1); Serial.println(F("kOhm"));
              #endif
              if (validateGas(kohm)) {
                gas->set(kohm);
              #ifdef SENSOR_BME680_DEBUG
              } else {
                Serial.println(F("BME680: gas reading failed validation"));
              #endif
              }
            #ifdef SENSOR_BME680_DEBUG
            } else {
              Serial.println(F("BME680: gas invalid or heater not stable - skipping gas"));
            #endif
            }
          }
        #ifdef SENSOR_BME680_DEBUG
        } else {
          Serial.println(F("BME680: reading failed validation"));
        #endif
        }
      }
    }
  }
}
