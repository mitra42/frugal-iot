#ifndef SENSOR_BH1750_H
#define SENSOR_BH1750_H

#include <Arduino.h>
#include "sensor_float.h"
#include <BH1750.h>

#ifndef SENSOR_BH1750_TOPIC
  #define SENSOR_BH1750_TOPIC "lux"
#endif
// TODO-115 practical range of lux unknown - apparantly can go from 0.001 to 65k
#define SENSOR_BH1750_ADVERTISEMENT "\n  -\n    topic: " SENSOR_BH1750_TOPIC "\n    name: Light\n    type: int\n    display: bar\n    min: 0\n    max: 65000\n    color: yellow\n    rw: r"

#ifndef SENSOR_BH1750_MS
  #define SENSOR_BH1750_MS 10000
#endif

#ifndef SENSOR_BH1750_ADDRESS
  #ifdef LILYGOHIGROW
    #define SENSOR_BH1750_ADDRESS (0x23)  // This may be a generally useful default ?
  #else
    #error Need to define SENSOR_BH1750_ADDRESS 
  #endif
#endif

class Sensor_BH1750 : public Sensor_Float {
  public:
    uint8_t pin;
    BH1750 lightmeter;
    Sensor_BH1750(const char* topic, uint8_t pin, const unsigned long ms, bool retain);
    void setup();
    virtual float read();
};
#endif // SENSOR_BH1750_H