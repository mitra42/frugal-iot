#ifndef SENSOR_DHT_H
#define SENSOR_DHT_H

#include "sensor_ht.h"
#include <dhtnew.h>                     // https://github.com/RobTillaart/DHTNew


#ifndef SENSOR_DHT_PIN
  #ifdef ESP8266_D1_MINI
    #define SENSOR_DHT_PIN D4
  #else 
    #ifdef LOLIN_C3_PICO
      #define SENSOR_DHT_PIN 6 // Currently untested but should be the same as D4 on ESP8266_D1_MINI
    #else
      #error No default pin for DHT sensor on your board
    #endif
  #endif 
#endif // SENSOR_DHT_PIN

#ifndef SENSOR_DHT_MS
  #define SENSOR_DHT_MS 60000
#endif

#define SENSOR_DHT_ADVERTISEMENT "\n  -\n    topic: temperature\n    name: Temperature\n    type: float\n    display: bar\n    min: 0\n    max: 45\n    color: red\n    rw: r\n  -\n    topic: humidity\n    name: Humidity\n    type: float\n    display: bar\n    min: 0\n    max: 100\n    color: cornflowerblue\n    rw: r"

class Sensor_DHT : public Sensor_HT {
public:
  uint8_t pin;
  DHTNEW *dht; 

  Sensor_DHT(const uint8_t pin, const char* topic, const char* topic2, const unsigned long ms);
  virtual void readAndSet(); // Combines function of set(read()) since reads two values from sensor
};

extern Sensor_DHT sensor_dht;

#endif // SENSOR_DHT_H
