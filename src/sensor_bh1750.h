#ifndef SENSOR_BH1750_H
#define SENSOR_BH1750_H

#include <Arduino.h>
#include "sensor_float.h"
#include <BH1750.h>

#ifndef SENSOR_BH1750_ID
  #define SENSOR_BH1750_ID "lux"
#endif
#ifndef SENSOR_BH1750_NAME
  #define SENSOR_BH1750_NAME "Lux"
#endif

#ifndef SENSOR_BH1750_MS
  #define SENSOR_BH1750_MS 10000
#endif

#ifndef SENSOR_BH1750_ADDRESS
  #ifdef LILYGOHIGROW
    #define SENSOR_BH1750_ADDRESS (0x23)  // This may be a generally useful default ?
  #else
    #define SENSOR_BH1750_ADDRESS 0x23  // This may be a generally useful default ?
    // #error Need to define SENSOR_BH1750_ADDRESS 
  #endif
#endif

// TODO-115 note there could be conflicts with other use of I2C and the Wire.h header which I think is where "Wire" is defined
#define I2C_SDA                 (25)
#define I2C_SCL                 (26)

class Sensor_BH1750 : public Sensor_Float {
  public:
    uint8_t pin;
    BH1750 lightmeter;
    Sensor_BH1750(const char* const id, const char * const name, uint8_t pin, const unsigned long ms, bool retain);
    void setup();
    virtual float read();
};
#endif // SENSOR_BH1750_H