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
    #define SENSOR_BH1750_ADDRESS 0x23  // This is the default also in the BH1750 library
  #endif
#endif

class Sensor_BH1750 : public Sensor_Float {
  public:
    Sensor_BH1750(const char* const id, const char * const name, const uint8_t addr, TwoWire* wire, const bool retain);
  protected:
    const uint8_t addr; // I2C address
    TwoWire* wire;
    BH1750 lightmeter;
    void setup() override;
    float readFloat() override;
    bool validate(float v) override;
};
#endif // SENSOR_BH1750_H