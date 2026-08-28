/* Frugal IoT - ENS160 air quality sensor (AQI, TVOC, eCO2)
 *
 * See sensor/ens160.h for the flags, the topics, and why this and sensor/aht.h were split out
 * of the old combined sensor/ens160aht21.cpp.
 *
 * Based on https://github.com/adafruit/ENS160_driver
 */

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "sensor/ens160.h"
#include "Frugal-IoT.h" // For frugal_iot (wiring the inputs in setup)

// Subset of the commands in https://github.com/adafruit/ENS160_driver/blob/master/src/ScioSense_ENS160.h
#define ENS160_BOOTING            10    // ms - a settling delay after each register write
#define ENS160_REG_PART_ID        0x00  // 2 byte register
#define ENS160_REG_OPMODE         0x10
#define ENS160_REG_COMMAND        0x12
#define ENS160_REG_TEMP_IN        0x13
#define ENS160_REG_DATA_STATUS    0x20
#define ENS160_REG_DATA_AQI       0x21
#define ENS160_REG_DATA_TVOC      0x22
#define ENS160_REG_DATA_ECO2      0x24
#define ENS160_COMMAND_NOP        0x00
#define ENS160_COMMAND_CLRGPR     0xCC
#define ENS160_DATA_STATUS_NEWDAT 0x02
//#define ENS160_OPMODE_DEP_SLEEP 0x00  // Not used yet - see the TODO in ens160.h
#define ENS160_OPMODE_IDLE        0x01
#define ENS160_OPMODE_STD         0x02
#define ENS160_OPMODE_RESET       0xF0
#define ENS160_PARTID             0x0160
#define ENS161_PARTID             0x0161
#define ENS160_LEN_DATA           7     // aqi[1] tvoc[2] eco2[2] aqi500[2], read in one go
#define ENS160_POLL_INTERVAL_MS   1

Sensor_ENS160::Sensor_ENS160(const char * const name, uint8_t address, TwoWire* wire, bool retain)
  : Sensor("ens160", name, retain),
    // The compensation values, until something arrives on the wire. Wireable, unlike the
    // outputs - the whole point of the split is that these can come from anywhere.
    temperature(new INfloat("ens160", "temperature", "Temperature", SENSOR_ENS160_DEFAULT_TEMPERATURE, 1, DEFAULT_ens160_temperature_min, DEFAULT_ens160_temperature_max, DEFAULT_ens160_temperature_color, true)),
    humidity(new INfloat("ens160", "humidity", "Humidity", SENSOR_ENS160_DEFAULT_HUMIDITY, 1, DEFAULT_ens160_humidity_min, DEFAULT_ens160_humidity_max, DEFAULT_ens160_humidity_color, true)),
    interface(address, wire)
{
  outputs.push_back(aqi = new OUTuint16("ens160", "aqi", "AQI", 0, DEFAULT_ens160_aqi_min, DEFAULT_ens160_aqi_max, DEFAULT_ens160_aqi_color, false));
  outputs.push_back(tvoc = new OUTuint16("ens160", "tvoc", "TVOC", 0, DEFAULT_ens160_tvoc_min, DEFAULT_ens160_tvoc_max, DEFAULT_ens160_tvoc_color, false));
  outputs.push_back(eco2 = new OUTuint16("ens160", "eco2", "eCO2", 0, DEFAULT_ens160_eco2_min, DEFAULT_ens160_eco2_max, DEFAULT_ens160_eco2_color, false));
  outputs.push_back(aqi500 = new OUTuint16("ens160", "aqi500", "AQI500", 0, DEFAULT_ens160_aqi500_min, DEFAULT_ens160_aqi500_max, DEFAULT_ens160_aqi500_color, false)); // ENS161 only
  tvoc->unit = "ppb";
  eco2->unit = "ppm";
}

// The chip wants a settling delay after each register write - Adafruit's driver calls this
// ENS_BOOTING and does the same. Note the return code is not to be relied on.
bool Sensor_ENS160::sendAndWait(uint8_t reg, uint8_t val) {
  bool status = interface.sendRegister(reg, val);
  #ifdef SENSOR_ENS160_DEBUG
    if (!status) {
      Serial.print(F("ENS160 failed to send reg=0x")); Serial.print(reg, HEX);
      Serial.print(F(" val=0x")); Serial.println(val, HEX);
    }
  #endif
  delay(ENS160_BOOTING);
  return status;
}
bool Sensor_ENS160::setMode(uint8_t val) {
  return sendAndWait(ENS160_REG_OPMODE, val);
}
bool Sensor_ENS160::command(uint8_t val) {
  return sendAndWait(ENS160_REG_COMMAND, val);
}
bool Sensor_ENS160::sendAndRead(uint8_t reg, uint8_t *buf, uint8_t num) {
  bool status = interface.sendAndRead(reg, buf, num);
  delay(ENS160_BOOTING);
  return status;
}

void Sensor_ENS160::setup() {
  temperature->setup(); // Before readConfigFromFS, which may carry a stored wired path
  humidity->setup();
  Sensor::setup();      // Will readConfigFromFS - do before touching the device
  // Only apply the compile-time defaults if nothing on the filesystem or in the UX wired them.
  // On the common ENS160+AHT21 board this is what joins the two halves back together.
  if (!temperature->wiredPath.length()) {
    temperature->wireTo(frugal_iot.messages->path(SENSOR_ENS160_TEMPERATURE_PATH));
  }
  if (!humidity->wiredPath.length()) {
    humidity->wireTo(frugal_iot.messages->path(SENSOR_ENS160_HUMIDITY_PATH));
  }
  #ifdef SENSOR_ENS160_DEBUG
    Serial.print(F("ENS160 compensation wired to ")); Serial.print(temperature->wiredPath);
    Serial.print(F(" and ")); Serial.println(humidity->wiredPath);
  #endif

  interface.initialize(); // calls wire->begin(), de-duplicated per bus
  setMode(ENS160_OPMODE_RESET);
  uint8_t readbuffer[2];
  sendAndRead(ENS160_REG_PART_ID, readbuffer, 2);
  uint16_t part_id = readbuffer[0] | ((uint16_t)readbuffer[1] << 8);
  #ifdef SENSOR_ENS160_DEBUG
    Serial.print(F("ENS160 partid=0x")); Serial.println(part_id, HEX);
  #endif
  // The part id used to be read only when debugging, which left isENS161 uninitialised in a
  // normal build - and with it, whether aqi500 was published at all.
  switch (part_id) {
    case ENS161_PARTID:
      isENS161 = true;
      break;
    case ENS160_PARTID:
      isENS161 = false;
      break;
    default:
      Serial.print(F("ENS160: unknown part id 0x")); Serial.print(part_id, HEX);
      Serial.println(F(" - check address and wiring"));
      setupFailed();
      break;
  }
  if (part_id == ENS160_PARTID || isENS161) {
    if (!isENS161) {
      // An ENS160 has no 0..500 index, so drop the output rather than discover a topic that
      // will only ever carry its initial 0. Safe here: discover() and any publishing happen
      // after every component's setup(), and IO::setup() is a no-op.
      for (auto it = outputs.begin(); it != outputs.end(); ++it) {
        if (*it == aqi500) {
          outputs.erase(it);
          break;
        }
      }
      delete aqi500;
      aqi500 = nullptr;
    }
    setMode(ENS160_OPMODE_IDLE);
    command(ENS160_COMMAND_NOP);
    command(ENS160_COMMAND_CLRGPR);
    setMode(ENS160_OPMODE_STD); // For TVOC and eCO2 rather than custom heater reads
    present = true;
    connected = true;
  }
}

void Sensor_ENS160::discover() {
  Sensor::discover(); // Outputs
  temperature->discover();
  humidity->discover();
}

void Sensor_ENS160::dispatch(System_Message &msg) {
  // The compensation values are published by *another* module, so this has to happen outside
  // any msg.module() == id test - compare Control::dispatch() and Sensor_DissolvedOxygen,
  // which do the same. Sensor::dispatch() wraps everything in that test, which is why this
  // cannot just be delegated upwards.
  temperature->dispatch(msg);
  humidity->dispatch(msg);
  Sensor::dispatch(msg);
}

// Sensor::captiveLines lists the outputs; the compensation values are inputs, so they are not
// in `outputs` and have to be added here - what is wired in matters when a reading looks wrong.
void Sensor_ENS160::captiveLines(AsyncResponseStream* response) {
  response->print(String(F("<p><label>")) + name + captiveValueLines()
    + "<br>Temperature (in): " + temperature->StringValue() + " C"
    + "<br>Humidity (in): " + humidity->StringValue() + " %</label></p>");
}

// Tell the chip the ambient conditions, so it can compensate its gas plate. Temperature is in
// 1/64 K and humidity in 1/512 %, both little-endian, written to consecutive registers.
bool Sensor_ENS160::setEnvData(float temp, float humy) {
  uint16_t t = (uint16_t)((temp + 273.15f) * 64.0f);
  uint16_t h = (uint16_t)(humy * 512.0f);
  uint8_t trh_in[5];
  trh_in[0] = ENS160_REG_TEMP_IN;
  trh_in[1] = t & 0xff;
  trh_in[2] = (t >> 8) & 0xff;
  trh_in[3] = h & 0xff;
  trh_in[4] = (h >> 8) & 0xff;
  return interface.send(trh_in, 5);
}

void Sensor_ENS160::readValidateConvertSet() {
  if (present) {
    setEnvData(temperature->floatValue(), humidity->floatValue());
    uint32_t start = millis(); // Not sleepSafeMillis as this is a sub-second wait
    uint8_t status;
    do {
      delay(ENS160_POLL_INTERVAL_MS);
      status = interface.send1read1(ENS160_REG_DATA_STATUS);
    } while (!(status & ENS160_DATA_STATUS_NEWDAT) && ((millis() - start) < SENSOR_ENS160_TIMEOUT_MS));
    uint8_t d[ENS160_LEN_DATA];
    if (!(status & ENS160_DATA_STATUS_NEWDAT)) {
      // A missing device reads 0xFF, which has the new-data bit set, so this is a chip that is
      // there but still converting - and one that is not there falls to the all-0xFF test below.
      connected = false;
      #ifdef SENSOR_ENS160_DEBUG
        Serial.println(F("ENS160: timed out waiting for a measurement"));
      #endif
    } else if (sendAndRead(ENS160_REG_DATA_AQI, d, ENS160_LEN_DATA)) {
      bool allff = true;
      for (uint8_t i = 0; i < ENS160_LEN_DATA; i++) {
        if (d[i] != 0xFF) {
          allff = false;
        }
      }
      if (allff) {
        connected = false;
        #ifdef SENSOR_ENS160_DEBUG
          Serial.println(F("ENS160: all 0xFF - device not responding"));
        #endif
      } else {
        connected = true;
        aqi->set(d[0]);
        tvoc->set(d[1] | ((uint16_t)d[2] << 8));
        eco2->set(d[3] | ((uint16_t)d[4] << 8));
        if (aqi500) { // ENS161 only - the output does not exist on an ENS160
          aqi500->set(((uint16_t)d[5]) | ((uint16_t)d[6] << 8));
        }
        #ifdef SENSOR_ENS160_DEBUG
          Serial.print(F("ENS160 aqi=")); Serial.print(d[0]);
          Serial.print(F(" tvoc=")); Serial.print(d[1] | ((uint16_t)d[2] << 8));
          Serial.print(F("ppb eco2=")); Serial.print(d[3] | ((uint16_t)d[4] << 8));
          Serial.print(F("ppm at ")); Serial.print(temperature->floatValue(), 1);
          Serial.print(F("C ")); Serial.print(humidity->floatValue(), 1); Serial.println(F("%"));
        #endif
      }
    }
  }
}
