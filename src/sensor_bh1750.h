#ifndef SENSOR_BH1750_H
#define SENSOR_BH1750_H

#include <Arduino.h>
#include "sensor_float.h"
#include <BH1750.h>

// Can use this default in constructor in main.cpp
#ifndef SENSOR_BH1750_ADDRESS
  #ifdef LILYGOHIGROW
    #define SENSOR_BH1750_ADDRESS (0x23)  // This may be a generally useful default ?
  #else
    #define SENSOR_BH1750_ADDRESS 0x23  // This may be a generally useful default ?
  #endif
#endif

// TODO-115 note there could be conflicts with other use of I2C and the Wire.h header which I think is where "Wire" is defined
// I think this is a lilygo specific thing - need to check with BH1750 on other boards
#define I2C_SDA                 (25)
#define I2C_SCL                 (26)

class Sensor_BH1750 : public Sensor_Float {
  public:
    uint8_t pin;
    BH1750 lightmeter;
    Sensor_BH1750(const char* const id, const char * const name, uint8_t pin, bool retain);
    void setup();
    virtual float read();
};
#endif // SENSOR_BH1750_H