/*
 * Temperature and Humidity sensor, 
 * Based on SHT85_demo_async.ino in https://github.com/RobTillaart/SHT85
 * 
 * Mitra Ardron: Sept 2024
 * 
 * Tested on Lolin SHT30 shield on ESP8266 - not yet tested on other devices or processors
 * 
 * Required:
 * SENSOR_SHT_DEVICE SHT30                  // Which kind of device, for now it presumes they are all the same.
 * SENSOR_SHT_ADDRESS                       // Device address
 * SENSOR_SHT_MS                            // How often to poll each sensor, for now we presume we poll them all this often
 * Optional: 
 * SENSOR_SHT_DEBUG                          // Debugging output
 *
 * TODO-16 Support multiple I2C Wires - so for example can use two sensors on each wire. See Issue#16
 * TODO-16 Pull the Wire support into a seperate module so that a single Wire can be used for alternate sensors. See Issue#16
 * TODO Support I2C multiplexors - see sample code at https://github.com/RobTillaart/SHT85/issues/26#issuecomment-2367448245
*/


#include "_settings.h"  // Settings for what to include etc

#ifdef SENSOR_SHT_WANT

#include <Arduino.h>
#include <SHT85.h>
#include "sensor_sht.h"
#include "system_mqtt.h"                // Library for sending messages


// TODO-130 add alternative constructor with id e.g. sht1, sht2 etc
Sensor_SHT::Sensor_SHT(const char* name, uint8_t address_init, TwoWire *wire, const unsigned long ms_init, bool retain) 
  : Sensor_HT("sht", name, ms_init, retain), 
    address(address_init) {
  //TODO-19b and TODO-16 It might be that we have to be careful to only setup the Wire once if there are multiple sensors. 
  Wire.begin(); // Appears to default to 4,5 which is correct for the Lolin D1 Mini SHT30 shield
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

//Sensor_SHT sensor_sht(SENSOR_SHT_ADDRESS, &Wire, "temperature", "humidity", SENSOR_SHT_MS);

#endif // SENSOR_SHT_WANT
