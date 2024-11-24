/*
 * Temperature and Humidity sensor, 
 *
 * Mitra Ardron: Nov 2024
 * 
 * Testing using the Arduino KY-015 board with its DHT11 - connected to Lolin D1 Mini 
 * 
 * 
 * Configuration options example - these are all in _configuration.h
 * Required:
 * SENSOR_DHT_PIN_ARRAY           // Which pins sensors connected to
 * SENSOR_DHT_MS                 // How often to poll each sensor, for now we presume we poll them all this often
 * Optional: 
 * SENSOR_DHT_DEBUG              // Debugging output
 * SENSOR_DHT_COUNT              // How many devices - default to 1
 * SENSOR_DHT_DEVICE             // Defaults to DHT11 - have not seen any others
 *
 * See examples at https://www.thegeekpub.com/wiki/sensor-wiki-ky-015-dht11-combination-temperature-and-humidity-sensor
 * Bit bashing at https://www.phippselectronics.com/using-the-temperature-and-humidity-sensor-ky-015-with-arduino/ looks simple as well
 * similar at https://arduinomodules.info/ky-015-temperature-humidity-sensor-module/ 
 * TODO-64 use non-blocking DHT at https://github.com/toannv17/DHT-Sensors-Non-Blocking/blob/main/DHT_Async.cpp (this is a library)
*/
// TODO-64 edits suggested for SHT - rename to SHT from SHT85, and change most SHTxx etc to SHT
// Use defaults for count (probably not device type as there are many)
// TODO-64 merge sensorDHT with sDHT - so that setup for example becomes static function of class

#include "_settings.h"  // Settings for what to include etc

#ifdef SENSOR_DHT_WANT

#if (!defined(SENSOR_DHT_PIN_ARRAY) || !defined(SENSOR_DHT_MS))
  error sensor_dht does not have all requirements in _configuration.h: SENSOR_DHT_PIN_ARRAY SENSOR_DHT_MS
#endif

#ifndef SENSOR_DHT_COUNT 
  #define SENSOR_DHT_COUNT 1
#endif
#ifndef SENSOR_DHT_DEVICE
  #define SENSOR_DHT_DEVICE DHT
#endif


#include <Arduino.h>
#include <dhtnew.h>
#include "sensor_dht.h"
#include "system_discovery.h"
#include "system_mqtt.h"                // Library for sending messages

class sensorDHT {
public:
  sensorDHT(uint8_t);
  void readSensor();
  DHTNEW *dht; 
  uint8_t pin;
  float temperature;
  float humidity;
protected:
};

namespace sDHT {

unsigned long nextLoopTime = 0;
sensorDHT *dht_array[SENSOR_DHT_COUNT];

#ifdef SENSOR_DHT_TOPIC_TEMPERATURE
  String *topicT;
#endif
#ifdef SENSOR_DHT_TOPIC_HUMIDITY
  String *topicH;
#endif

void setup()
{
  uint8_t dht_pin_array[] = {SENSOR_DHT_PIN_ARRAY};

  #ifdef SENSOR_DHT_TOPIC_TEMPERATURE
    topicT = new String(*xDiscovery::topicPrefix + F(SENSOR_DHT_TOPIC_TEMPERATURE));
  #endif
  #ifdef SENSOR_DHT_TOPIC_HUMIDITY
    topicH = new String(*xDiscovery::topicPrefix + F(SENSOR_DHT_TOPIC_HUMIDITY));
  #endif

  for(uint8_t i = 0 ; i < SENSOR_DHT_COUNT; i++) {
    dht_array[i] = new sensorDHT(dht_pin_array[i]);
  }

}

void loop() {
  if (nextLoopTime <= millis() ) {
    for(uint8_t i = 0 ; i < SENSOR_DHT_COUNT; i++) {
      dht_array[i]->readSensor();
    }
    nextLoopTime = millis() + SENSOR_DHT_MS;
  }
}

} // namespace sDHT

sensorDHT::sensorDHT(uint8_t p) {
    dht = new DHTNEW(p); //TODO-64 is the library working for other DHTs - check other examples at https://github.com/RobTillaart/DHTNew/tree/master/examples
    dht->setType(11); // Override bug in DHTnew till fixed see https://github.com/RobTillaart/DHTNew/issues/102
    temperature = 0;
    humidity = 0; 
    #ifdef SENSOR_DHT_DEBUG
      pin = p; // Just copy for debugging
    #endif // SENSOR_DHT_DEBUG
}

#ifdef SENSOR_DHT_DEBUG
void printErrorCode(int chk) {
  switch (chk)
  {
    case DHTLIB_OK:
      Serial.print(F("OK,\t"));
      break;
    case DHTLIB_ERROR_CHECKSUM:
      Serial.print(F("Checksum error,\t"));
      break;
    case DHTLIB_ERROR_TIMEOUT_A:
      Serial.print(F("Time out A error,\t"));
      break;
    case DHTLIB_ERROR_TIMEOUT_B:
      Serial.print(F("Time out B error,\t"));
      break;
    case DHTLIB_ERROR_TIMEOUT_C:
      Serial.print(F("Time out C error,\t"));
      break;
    case DHTLIB_ERROR_TIMEOUT_D:
      Serial.print(F("Time out D error,\t"));
      break;
    case DHTLIB_ERROR_SENSOR_NOT_READY:
      Serial.print(F("Sensor not ready,\t"));
      break;
    case DHTLIB_ERROR_BIT_SHIFT:
      Serial.print(F("Bit shift error,\t"));
      break;
    case DHTLIB_WAITING_FOR_READ:
      Serial.print(F("Waiting for read,\t"));
      break;
    default:
      Serial.print(F("Unknown: "));
      Serial.print(chk);
      Serial.print(F(",\t"));
      break;
  }
}
#endif // SHT_DHT_DEBUG

void sensorDHT::readSensor() {
  #ifdef SENSOR_DHT_DEBUG
    Serial.print("DHT");
    Serial.print(pin);
    Serial.print(F("   "));
  #endif

  int chk = dht->read();
  #ifdef SENSOR_DHT_DEBUG
    printErrorCode(chk);
  #endif
  if (!chk) { // Dont read if error

    float temp = dht->getTemperature();
    float humy = dht->getHumidity();

    // Note, not smoothing the data as it seems fairly stable and is float rather than bits anyway
    #ifdef SENSOR_DHT_DEBUG
      Serial.print(temp, 1);
      Serial.print(F("°C\t"));
      Serial.print(humy, 1);
      Serial.println(F("%"));
    #endif

    // Store new results and optionally if changed send on MQTT
    #ifdef SENSOR_DHT_TOPIC_TEMPERATURE
      if (temp != temperature) {
        xMqtt::messageSend(*sDHT::topicT, temp, 1, false, 0);
      }
    #endif

    temperature = temp;
    #ifdef SENSOR_DHT_TOPIC_HUMIDITY
      if (humy != humidity) { // TODO may want to add some bounds (e.g a percentage)
        xMqtt::messageSend(*sDHT::topicH, humy, 1, false, 0);
      }
    #endif
    humidity = humy;

  }
}

#endif // SENSOR_DHT_WANT

//  -- END OF FILE --
