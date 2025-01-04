#ifndef SENSOR_SHT_H
#define SENSOR_SHT_H


#include <SHT85.h>
#include "sensor_ht.h"

#ifndef SENSOR_SHT_DEVICE
  #define SENSOR_SHT_DEVICE SHT30 // e.g. The Lolin SHT30 shield
#endif

#define SENSOR_SHT_ADVERTISEMENT "\n  -\n    topic: temperature\n    name: Temperature\n    type: float\n    display: bar\n    min: 0\n    max: 45\n    color: red\n    rw: r\n  -\n    topic: humidity\n    name: Humidity\n    type: float\n    display: bar\n    min: 0\n    max: 100\n    color: cornflowerblue\n    rw: r"

class Sensor_SHT : public Sensor_HT {
public:
  uint8_t address;
  SENSOR_SHT_DEVICE *sht; 
  Sensor_SHT(uint8_t address, TwoWire *wire, const char* topic, const char* topic2, const unsigned long ms);
  virtual void readAndSet(); // Combines function of set(read()) since read gets two values from sensor // TODO-25 maybe combine as virtual function on Sensor
};

extern Sensor_SHT sensor_sht;

#endif // SENSOR_SHT_H
