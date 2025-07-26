/*
 * Temperature and Humidity sensor, 
 * Based on SHT85_demo_async.ino in https://github.com/RobTillaart/SHT85
 * 
 * Mitra Ardron: Sept 2024...Jun 2025
 * 
 * See the for guide to building one at 
 * https://github.com/mitra42/frugal-iot/wiki/Building-a-temperature---humidity-sensor/ 
 * 
 * Tested on:
 * Sensors: Lolin SHT30; a nice one on a cord, including some really cheap ones - no known compatability issues
 * Dev boards: ESP8266 & ESP32 on multiple boards - no known compatability issues
 * 
 * Required
 * Optional (default)
 * SENSOR_SHT_DEVICE (SHT30) // Which kind of device, for now it presumes they are all the same.
 * SENSOR_SHT_DEBUG          // Debugging output
 * 
 * Note - following is only used in main.cpp for constructor wth default in .h
 * SENSOR_SHT_ADDRESS (0x44) // Device address - 0x45 is also common esp D1 SHT shield
 *
 *
 * TODO-16 Support multiple I2C Wires - so for example can use two sensors on each wire. See Issue#16
 * TODO-16 Pull the Wire support into a seperate module so that a single Wire can be used for alternate sensors. See Issue#16
 * TODO Support I2C multiplexors - see sample code at https://github.com/RobTillaart/SHT85/issues/26#issuecomment-2367448245
*/


#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include <SHT85.h>
#include "sensor_sht.h"

// TODO Add alternative constructor with id e.g. sht1, sht2 etc
Sensor_SHT::Sensor_SHT(const char * const name, uint8_t address_init, TwoWire *wire, bool retain) 
  : Sensor_HT("sht", name, retain), 
    address(address_init) {
  //TODO move most of this to setup rather than constructor
  //
  //TODO-19b and TODO-16 It might be that we have to be careful to only setup the Wire once if there are multiple sensors. 
  // Defaults to system defined SDA and SCL 
  // TODO this will fail if the argument `wire` is not `&Wire`
  Wire.begin(I2C_SDA, I2C_SCL);  // These are defined in _settings.h as SDA and SCL unless there is board specifics
  Wire.setClock(100000);

  sht = new SENSOR_SHT_DEVICE(address, wire);
  sht->begin();
  #ifdef SENSOR_SHT_DEBUG
    Serial.print(F("address: ")); Serial.print(address, HEX);
    Serial.print(F(" status: ")); Serial.print(sht->readStatus(), HEX);
    Serial.println();
  #endif // SENSOR_SHT_DEBUG
  sht->requestData(); // Initial request queued up  (loop is to read data and queue up next read)
}

void Sensor_SHT::readAndSet() {
  #ifdef SENSOR_SHT_DEBUG
    Serial.print(address, HEX);
    Serial.print(F("   "));
  #endif

  // There is an implicit asssumption here that sensors should be able to go from requestData to dataReady between loops -
  // and if not it will be reported and read on next loop
  if (sht->dataReady())
  {
    if (sht->readData()) {
      float temp = sht->getTemperature(); // TODO use raw version https://github.com/RobTillaart/SHT85
      float humy = sht->getHumidity(); // TODO use raw version https://github.com/RobTillaart/SHT85
      // Note, not smoothing the data as it seems fairly stable and is float rather than bits anyway
      #ifdef SENSOR_SHT_DEBUG
        Serial.print(temp, 1);
        Serial.print(F("°C\t"));
        Serial.print(humy, 1);
        Serial.println(F("%"));
      #endif

      set(temp, humy); // Set the values in the OUT object and send

      // Note only request more Data if was dataReady
      sht->requestData(); // Request next one

    #ifdef SENSOR_SHT_DEBUG
    } else {
      Serial.println(F("SHT sensor did not return data"));
    #endif // SENSOR_SHT_DEBUG
    }
  #ifdef SENSOR_SHT_DEBUG
  } else {
    Serial.println(F("SHT sensor not ready"));
  #endif
  }
}
