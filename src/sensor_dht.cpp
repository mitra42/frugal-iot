/* Frugal IoT - DHT temperature and humidity sensor
 * 
 * Mitra Ardron: Nov 2024
 * 
 * Testing using the Arduino KY-015 board with its DHT11 - connected to Lolin D1 Mini 
 * 
 * Required:
 * Optional: 
 * SENSOR_DHT_DEBUG              // Debugging output
 *
 * See examples at https://www.thegeekpub.com/wiki/sensor-wiki-ky-015-dht11-combination-temperature-and-humidity-sensor
 * Bit bashing at https://www.phippselectronics.com/using-the-temperature-and-humidity-sensor-ky-015-with-arduino/ looks simple as well
 * similar at https://arduinomodules.info/ky-015-temperature-humidity-sensor-module/ 
 * or non-blocking DHT at https://github.com/toannv17/DHT-Sensors-Non-Blocking/blob/main/DHT_Async.cpp (this is a library)
 *
 * This version uses Rob Tillart's library (who also did the SHT library we use) 
 * 
 * Known Pins for various boards
 *  LilyGo HiGrow:        GPIO_NUM_16
 *  ARDUINO_LOLIN_C3_PICO 6 // Currently untested but should be the same physical pin as D4 on ESP8266_D1
 *  ESP8266_D1            D4 // Standard on DHT D1 Mini shields
 * 
 * Include in main.cpp as e.g.
 *    frugal_iot.sensors->add(new Sensor_DHT("DHT", GPIO_NUM_16, true));
 * 
*/

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include <dhtnew.h> // https://github.com/RobTillaart/DHTNew
#include "sensor_dht.h"

// Add alternative constructor with id e.g. dht1, dht2 etc
Sensor_DHT::Sensor_DHT(const char * const name, const uint8_t pin_init, bool retain) 
  : Sensor_HT("dht", name, retain), 
    dht(new DHTNEW(pin_init)),
   pin(pin_init) {
  //TODO-64 is the library working for other DHTs - check other examples at https://github.com/RobTillaart/DHTNew/tree/master/examples
  // dht->setType(11); // Override bug in DHTnew till fixed see https://github.com/RobTillaart/DHTNew/issues/104
}
void Sensor_DHT::setup() {
  dht->powerUp(); //TODO-POWER think about when do this
  Sensor_HT::setup(); // Call parent setup
}

#ifdef SENSOR_DHT_DEBUG
void printErrorCode(int chk) {
  switch (chk)
  {
    case DHTLIB_OK:
      Serial.print(F("OK,\t"));
      break;
    case DHTLIB_ERROR_CHECKSUM:
      Serial.println(F("Checksum error,\t"));
      break;
    case DHTLIB_ERROR_TIMEOUT_A:
      Serial.println(F("Time out A error,\t"));
      break;
    case DHTLIB_ERROR_TIMEOUT_B:
      Serial.println(F("Time out B error,\t"));
      break;
    case DHTLIB_ERROR_TIMEOUT_C:
      Serial.println(F("Time out C error,\t"));
      break;
    case DHTLIB_ERROR_TIMEOUT_D:
      Serial.println(F("Time out D error,\t"));
      break;
    case DHTLIB_ERROR_SENSOR_NOT_READY:
      Serial.println(F("Sensor not ready,\t"));
      break;
    case DHTLIB_ERROR_BIT_SHIFT:
      Serial.println(F("Bit shift error,\t"));
      break;
    case DHTLIB_WAITING_FOR_READ:
      Serial.println(F("Waiting for read,\t"));
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
    Serial.print(F("DHT"));
    Serial.print(dht->getType());
    Serial.print(F(" on "));
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

    set(temp, humy); // Will also send message via output->set()
  }
}
