#ifndef SENSOR_DHT_H
#define SENSOR_DHT_H

#include "sensor_ht.h"
#include <dhtnew.h>                     // https://github.com/RobTillaart/DHTNew


#if defined(SENSOR_DHT_WANT) && !defined(SENSOR_DHT_PIN)
  #ifdef ESP8266_D1
    #define SENSOR_DHT_PIN D4
  #elif defined(LOLIN_C3_PICO)
    #define SENSOR_DHT_PIN 6 // Currently untested but should be the same as D4 on ESP8266_D1
  #elif defined(LILYGOHIGROW)
    #define SENSOR_DHT_PIN GPIO_NUM_16
  #else
    #error No default pin for DHT sensor on your board
  #endif
#endif // SENSOR_DHT_PIN

class Sensor_DHT : public Sensor_HT {
public:
  uint8_t pin;
  DHTNEW *dht; 

  Sensor_DHT(const char * const name, const uint8_t pin, bool retain);
  virtual void readAndSet(); // Combines function of set(read()) since reads two values from sensor
};

extern Sensor_DHT sensor_dht;

#endif // SENSOR_DHT_H
