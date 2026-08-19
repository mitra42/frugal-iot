/* Frugal IoT - AHT20 / AHT21 temperature and humidity sensor
 *
 * See sensor/aht.h for the flags, and for why the two chips share one implementation.
 *
 * This is the AHT half of what used to be sensor/ens160aht21.cpp, which handled both chips on
 * the common ENS160+AHT21 breakout in a single class. The ENS160 half is now sensor/ens160.h,
 * which takes temperature and humidity as *inputs* - so the pair are wired together in the
 * sketch (see examples/ensaht) rather than welded together in C++, and either chip can now be
 * used on its own.
 *
 * Thanks for lessons learned and some ideas/bits copied from
 *   https://github.com/adafruit/Adafruit_AHTX0   Copyright (c) Adafruit, MIT licence
 * //TODO-101 also review https://registry.platformio.org/libraries/k0i05/esp_ahtxx
 */

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "sensor/aht.h"
#include "cmath" // for std::isnan

#define AHT_CMD_TRIGGER         0xAC // Followed by 0x33 0x00
#define AHT_CMD_TRIGGER_ARG1    0x33
#define AHT_CMD_TRIGGER_ARG2    0x00
#define AHT_CMD_SOFTRESET       0xBA
#define AHT_CMD_INIT_ARG1       0x08
#define AHT_CMD_INIT_ARG2       0x00
#define AHT_STATUS_REGISTER     0x71 // Only in K0I05's driver - not in Adafruit's
#define AHT_STATUS_BUSY         0x80
#define AHT_STATUS_CALIBRATED   0x08
#define AHT_LEN_DATA            6
#define AHT_POLL_INTERVAL_MS    10

Sensor_AHT::Sensor_AHT(const char * const id, const char * const name, uint8_t address,
  TwoWire* wire, bool retain, const OutputRange temperatureRange, const OutputRange humidityRange)
  : Sensor(id, name, retain),
    // The ranges come from the subclass, so each chip gets its own module's UX defaults
    temperature(new OUTfloat(id, "temperature", "Temperature", 0, 1, temperatureRange.min, temperatureRange.max, temperatureRange.color, false)),
    humidity(new OUTfloat(id, "humidity", "Humidity", 0, 1, humidityRange.min, humidityRange.max, humidityRange.color, false)),
    interface(address, wire)
{
  outputs.push_back(temperature);
  outputs.push_back(humidity);
  temperature->unit = "C";
  humidity->unit = "%";
}

// Poll the status register until the chip clears its busy bit. A missing device reads back
// 0xFF, whose busy bit is set, so this must time out rather than spin - `ok` says which
// happened. The status is returned for the caller's own bit tests.
uint8_t Sensor_AHT::spinTillReady(bool &ok) {
  uint32_t start = millis(); // Not sleepSafeMillis as this is a sub-second wait
  uint8_t status;
  do {
    delay(AHT_POLL_INTERVAL_MS);
    status = interface.send1read1(AHT_STATUS_REGISTER);
  } while ((status & AHT_STATUS_BUSY) && ((millis() - start) < SENSOR_AHT_TIMEOUT_MS));
  ok = !(status & AHT_STATUS_BUSY);
  #ifdef SENSOR_AHT_DEBUG
    if (!ok) {
      Serial.print(id); Serial.print(F(": still busy after ")); Serial.print(SENSOR_AHT_TIMEOUT_MS);
      Serial.print(F("ms, status=0x")); Serial.println(status, HEX);
    }
  #endif
  return status;
}

void Sensor_AHT::setup() {
  Sensor::setup(); // Will readConfigFromFS - do before touching the device
  interface.initialize(); // calls wire->begin(), de-duplicated per bus
  bool ok;
  interface.send(AHT_CMD_SOFTRESET);
  spinTillReady(ok); // Just waiting for not busy
  if (!ok) {
    Serial.print(id); Serial.println(F(": no response - check address and wiring"));
    setupFailed();
  } else {
    // See note in aht.h - the datasheets say 0xBE, Adafruit (and this board) send 0xE1, and
    // these parts are factory calibrated either way.
    uint8_t cmd[3] = { SENSOR_AHT_CMD_INIT, AHT_CMD_INIT_ARG1, AHT_CMD_INIT_ARG2 };
    if (!interface.send(cmd, 3)) {
      #ifdef SENSOR_AHT_DEBUG
        Serial.print(id); Serial.println(F(": initialisation command was not acknowledged"));
      #endif
    }
    uint8_t status = spinTillReady(ok);
    #ifdef SENSOR_AHT_DEBUG
      Serial.print(id); Serial.print(F(" status (wanting & 0x08): 0x")); Serial.println(status, HEX);
    #endif
    // Not calibrated is worth saying out loud - the readings will be wrong rather than absent -
    // but it is not fatal, so carry on and let the values speak for themselves.
    if (ok && !(status & AHT_STATUS_CALIBRATED)) {
      Serial.print(id); Serial.println(F(": reports itself uncalibrated"));
    }
    present = true;
    connected = true;
  }
}

// The chip's own range is -40..85C and 0..100%, and its characteristic failure is to return
// zeros for both, which is a plausible reading in a cold room - reject the pair.
bool Sensor_AHT::validate(float temp, float humy) {
  return !std::isnan(temp) && !std::isnan(humy)
      && (temp > -40.0f) && (temp < 85.0f)
      && (humy >= 0.0f) && (humy <= 100.0f)
      && !((temp == 0.0f) && (humy == 0.0f));
}

void Sensor_AHT::readValidateConvertSet() {
  if (present) {
    uint8_t cmd[3] = { AHT_CMD_TRIGGER, AHT_CMD_TRIGGER_ARG1, AHT_CMD_TRIGGER_ARG2 };
    uint8_t data[AHT_LEN_DATA];
    bool ok;
    interface.send(cmd, 3);
    spinTillReady(ok);
    if (!ok || !interface.read(data, AHT_LEN_DATA)) {
      connected = false;
      #ifdef SENSOR_AHT_DEBUG
        Serial.print(id); Serial.println(F(": failed to read"));
      #endif
    } else {
      // From the Adafruit library, and agreeing with K0I05's:
      //   humidity    = (rx[1] << 12) | (rx[2] << 4) | (rx[3] >> 4)
      //   temperature = ((rx[3] & 0x0f) << 16) | (rx[4] << 8) | rx[5]
      uint32_t h = data[1];
      h <<= 8;
      h |= data[2];
      h <<= 4;
      h |= data[3] >> 4;
      uint32_t t = data[3] & 0x0F;
      t <<= 8;
      t |= data[4];
      t <<= 8;
      t |= data[5];
      float humy = ((float)h * 100) / 0x100000;
      float temp = ((float)t * 200) / 0x100000 - 50;
      #ifdef SENSOR_AHT_DEBUG
        Serial.print(id); Serial.print(F(" ")); Serial.print(temp, 1); Serial.print(F("C "));
        Serial.print(humy, 1); Serial.println(F("%"));
      #endif
      if (validate(temp, humy)) {
        connected = true;
        temperature->set(temp);
        humidity->set(humy);
      #ifdef SENSOR_AHT_DEBUG
      } else {
        Serial.print(id); Serial.println(F(": reading failed validation"));
      #endif
      }
    }
  }
}

Sensor_AHT20::Sensor_AHT20(const char * const name, uint8_t address, TwoWire* wire, bool retain)
  : Sensor_AHT("aht20", name, address, wire, retain,
      {DEFAULT_aht20_temperature_min, DEFAULT_aht20_temperature_max, DEFAULT_aht20_temperature_color},
      {DEFAULT_aht20_humidity_min, DEFAULT_aht20_humidity_max, DEFAULT_aht20_humidity_color})
  { }

Sensor_AHT21::Sensor_AHT21(const char * const name, uint8_t address, TwoWire* wire, bool retain)
  : Sensor_AHT("aht21", name, address, wire, retain,
      {DEFAULT_aht21_temperature_min, DEFAULT_aht21_temperature_max, DEFAULT_aht21_temperature_color},
      {DEFAULT_aht21_humidity_min, DEFAULT_aht21_humidity_max, DEFAULT_aht21_humidity_color})
  { }
