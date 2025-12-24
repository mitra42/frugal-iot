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

#ifndef SENSOR_DHT_H
#define SENSOR_DHT_H

#include "sensor_ht.h"
#include <dhtnew.h>                     // https://github.com/RobTillaart/DHTNew


class Sensor_DHT : public Sensor_HT {
public:
  Sensor_DHT(const char * const name, const uint8_t pin, const bool retain);
protected:
  DHTNEW * const dht; 
  const uint8_t pin;
  void setup() override;
  void readValidateConvertSet() override; // Combines function of set(read()) since reads two values from sensor
};

extern Sensor_DHT sensor_dht;

#endif // SENSOR_DHT_H
