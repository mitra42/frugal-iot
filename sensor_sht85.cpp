/*
 * Temperature and Humidity sensor, 
 * Based on SHT85_demo_async.ino in https://github.com/RobTillaart/SHT85
 *
 * Mitra Ardron: Sept 2024
 * 
 * Tested on Lolin SHT30 shield on ESP8266 - not yet tested on other devices or processors
 * 
 * Configuration options example - these are all in _configuration.h
 * Required:
 * SENSOR_SHT85_DEVICE SHT30                  // Which kind of device, for now it presumes they are all the same.
 * SENSOR_SHT85_ADDRESS_ARRAY 0x45,0x44       // A list of device addresses
 * SENSOR_SHT85_COUNT                         // How many devices
 * SENSOR_SHT85_MS                            // How often to poll each sensor, for now we presume we poll them all this often
 * Optional: 
 * SENSOR_SHT85_DEBUG                          // Debugging output
 *
 * TODO-16 Support multiple I2C Wires - so for example can use two sensors on each wire. See Issue#16
 * TODO-16 Pull the Wire support into a seperate module so that a single Wire can be used for alternate sensors. See Issue#16
 * TODO Support I2C multiplexors - see sample code at https://github.com/RobTillaart/SHT85/issues/26#issuecomment-2367448245
*/

#include "_settings.h"  // Settings for what to include etc

#ifdef SENSOR_SHT85_WANT

#if (!defined(SENSOR_SHT85_DEVICE) || !defined(SENSOR_SHT85_ADDRESS_ARRAY) || !defined(SENSOR_SHT85_COUNT) || !defined(SENSOR_SHT85_MS))
  error sensor_sht85 does not have all requirements in _configuration.h: SENSOR_SHT85_DEVICE SENSOR_SHT85_ADDRESS_ARRAY SENSOR_SHT85_COUNT SENSOR_SHT85_MS 
#endif


#include <Arduino.h>
#include <SHT85.h>
#include "sensor_sht85.h"
#include "system_discovery.h"
#include "system_mqtt.h"                // Library for sending messages

class sSHTxx {
public:
  sSHTxx(uint8_t address, TwoWire *wire = &Wire);
  void readSensor();
  SENSOR_SHT85_DEVICE *sht; 
  uint8_t address;
  float temperature;
  float humidity;
protected:
};

namespace sSHT85 {

unsigned long nextLoopTime = 0;
sSHTxx *sht_array[SENSOR_SHT85_COUNT];

#ifdef SENSOR_SHT85_TOPIC_TEMPERATURE
  String *topicT;
#endif
#ifdef SENSOR_SHT85_TOPIC_HUMIDITY
  String *topicH;
#endif

void setup()
{
  uint8_t sht_address_array[] = {SENSOR_SHT85_ADDRESS_ARRAY};

  #ifdef SENSOR_SHT85_TOPIC_TEMPERATURE
    topicT = new String(*xDiscovery::topicPrefix + F(SENSOR_SHT85_TOPIC_TEMPERATURE));
  #endif
  #ifdef SENSOR_SHT85_TOPIC_HUMIDITY
    topicH = new String(*xDiscovery::topicPrefix + F(SENSOR_SHT85_TOPIC_HUMIDITY));
  #endif

  //TODO-19b and TODO-16 It might be that we have to be careful to only setup the Wire once if there are multiple sensors. 
  Wire.begin(); // Appears to default to 4,5 which is correct for the Lolin D1 Mini SHT30 shield
  Wire.setClock(100000);


  for(uint8_t i = 0 ; i < SENSOR_SHT85_COUNT; i++) {
    sht_array[i] = new sSHTxx(sht_address_array[i], &Wire);
  }

}

void loop() {
  if (nextLoopTime <= millis() ) {
    for(uint8_t i = 0 ; i < SENSOR_SHT85_COUNT; i++) {
      sht_array[i]->readSensor();
    }
    nextLoopTime = millis() + SENSOR_SHT85_MS;
  }
}

} // namespace sSHT85

sSHTxx::sSHTxx(uint8_t addr, TwoWire *wire) {

    sht = new SENSOR_SHT85_DEVICE(addr, wire);
    temperature = 0;
    humidity = 0; 
    sht->begin();
    #ifdef SENSOR_SHT85_DEBUG
      address = addr; // Just copy for debugging
      Serial.print(F("addr: ")); Serial.print(addr, HEX);
      Serial.print(F(" status: ")); Serial.print(sht->readStatus(), HEX);
      Serial.println();
    #endif // SENSOR_SHT85_DEBUG
    sht->requestData(); // Initial request queued up  (loop is to read data and queue up next read)
}

void sSHTxx::readSensor() {
  #ifdef SENSOR_SHT85_DEBUG
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
      #ifdef SENSOR_SHT85_DEBUG
        Serial.print(temp, 1);
        Serial.print(F("°C\t"));
        Serial.print(humy, 1);
        Serial.println(F("%"));
      #endif

      // Store new results and optionally if changed send on MQTT
      #ifdef SENSOR_SHT85_TOPIC_TEMPERATURE
      if (temp != temperature) {
        xMqtt::messageSend(*sSHT85::topicT, temp, 1, false, 0);
      }
      #endif
      temperature = temp;
      #ifdef SENSOR_SHT85_TOPIC_HUMIDITY
        if (humy != humidity) { // TODO may want to add some bounds (e.g a percentage)
          xMqtt::messageSend(*sSHT85::topicH, humy, 1, false, 0);
        }
      #endif
      humidity = humy;

      // Note only request more Data if was dataReady
      sht->requestData(); // Request next one


    #ifdef SENSOR_SHT85_DEBUG
    } else {
      Serial.println(F("SHT sensor did not return data"));
    #endif // SENSOR_SHT85_DEBUG
    }
  #ifdef SENSOR_SHT85_DEBUG
  } else {
    Serial.println(F("SHT sensor not ready"));
  #endif
  }
}

#endif // SENSOR_SHT85_WANT

//  -- END OF FILE --
