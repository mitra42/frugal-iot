#ifndef SENSOR_DHT_H
#define SENSOR_DHT_H

#include "sensor_ht.h"
#include <dhtnew.h>                     // https://github.com/RobTillaart/DHTNew


#ifndef SENSOR_DHT_PIN
  #define SENSOR_DHT_PIN 4  // hard wired to a pin - 4 is D2 on the Lolin D1 but is somewhere else on C3 Pico
#endif

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
