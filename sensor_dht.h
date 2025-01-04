#ifndef SENSOR_DHT_H
#define SENSOR_DHT_H

#include "sensor_ht.h"
#include <dhtnew.h>                     // https://github.com/RobTillaart/DHTNew

#define SENSOR_DHT_ADVERTISEMENT "\n  -\n    topic: temperature\n    name: Temperature\n    type: float\n    display: bar\n    min: 0\n    max: 45\n    color: red\n    rw: r\n  -\n    topic: humidity\n    name: Humidity\n    type: float\n    display: bar\n    min: 0\n    max: 100\n    color: cornflowerblue\n    rw: r"

class Sensor_DHT : public Sensor_HT {
public:
  uint8_t pin;
  DHTNEW *dht; 

  Sensor_DHT(const uint8_t pin, const char* topic, const char* topic2, const unsigned long ms);
  virtual void readAndSet(); // Combines function of set(read()) since read gets two values from sensor // TODO-25 maybe combine as virtual function on Sensor
};

extern Sensor_DHT sensor_dht;

#endif // SENSOR_DHT_H
