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
 * Optional: 
 * SENSOR_DHT_MS                 // How often to poll each sensor, for now we presume we poll them all this often
 * SENSOR_DHT_PIN          // Which pins sensors connected to - default to 4
 * SENSOR_DHT_DEBUG              // Debugging output
 * SENSOR_DHT_COUNT              // How many devices - default to 1
 *
 * See examples at https://www.thegeekpub.com/wiki/sensor-wiki-ky-015-dht11-combination-temperature-and-humidity-sensor
 * Bit bashing at https://www.phippselectronics.com/using-the-temperature-and-humidity-sensor-ky-015-with-arduino/ looks simple as well
 * similar at https://arduinomodules.info/ky-015-temperature-humidity-sensor-module/ 
 * or non-blocking DHT at https://github.com/toannv17/DHT-Sensors-Non-Blocking/blob/main/DHT_Async.cpp (this is a library)
 *
 * This version uses Rob Tillart's library (who also did the SHT library we use) 
*/
// TODO-64 edits suggested for SHT - rename to SHT from SHT85, and change most SHTxx etc to SHT
// Use defaults for count (probably not device type as there are many)
// TODO-64 merge sensorDHT with sDHT - so that setup for example becomes static function of class

#include "_settings.h"  // Settings for what to include etc

#ifdef SENSOR_DHT_WANT

#ifndef SENSOR_DHT_PIN
  #define SENSOR_DHT_PIN 4  // hard wired to a pin - 4 is D2 on the Lolin D1 but is somewhere else on C3 Pico
#endif

#ifndef SENSOR_DHT_MS
  #define SENSOR_DHT_MS 60000
#endif

#include <Arduino.h>
#include <dhtnew.h>                     // https://github.com/RobTillaart/DHTNew
#include "sensor_dht.h"
#include "system_mqtt.h"                // Library for sending messages

Sensor_DHT::Sensor_DHT(const uint8_t pin_init, const char* topic_init, const char* topic2_init, const unsigned long ms_init) : Sensor_Float(topic_init, ms_init) { 
  pin = pin_init;
  topic2 = topic2_init;
  temperature = 0;
  humidity = 0; 
  dht = new DHTNEW(pin_init); //TODO-64 is the library working for other DHTs - check other examples at https://github.com/RobTillaart/DHTNew/tree/master/examples
  // dht->setType(11); // Override bug in DHTnew till fixed see https://github.com/RobTillaart/DHTNew/issues/104
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

void Sensor_DHT::readAndSet() {
  #ifdef SENSOR_DHT_DEBUG
    Serial.print("DHT");
    Serial.print(pin);
    Serial.print(F("   "));
  #endif

  int chk = dht->read();
  #ifdef SENSOR_DHT_DEBUG
    printErrorCode(chk);
    //Serial.println(dht->getType());
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
    if (temp != temperature) {
      temperature = temp;
      if (topic) {
        xMqtt::messageSend(topic, temperature, 1, false, 0);  // topic, value, width, retain, qos
      }
    }
    if (humy != humidity) { // TODO may want to add some bounds (e.g a percentage)
      humidity = humy;
      if (topic2) {
        xMqtt::messageSend(topic2, humidity, 1, false, 0);
      }
    }
  }
}

void Sensor_DHT::loop() { // TODO-25 I think this is standard esp since readAndSet is type independent while read() and set(xxx) are type dependent
  if (nextLoopTime <= millis()) {
    readAndSet(); // Will also send message via act()
    nextLoopTime = millis() + ms;
  }
}

Sensor_DHT sensor_dht(SENSOR_DHT_PIN, "temperature", "humidity", SENSOR_DHT_MS);

#endif // SENSOR_DHT_WANT

//  -- END OF FILE --
