#ifndef SENSOR_BH1750_H
#define SENSOR_BH1750_H

#include <Arduino.h>

// Include the superclass
#include "sensor_float.h"

// Include the library we are building on
#include <BH1750.h>

// Define a defauly for the sensor address that can be used in the main.cpp or *.ino file
#ifndef SENSOR_BH1750_ADDRESS
  #ifdef LILYGOHIGROW
    #define SENSOR_BH1750_ADDRESS (0x23)  // This may be a generally useful default ?
  #else
    #define SENSOR_BH1750_ADDRESS 0x23  // This is the default also in the BH1750 library
  #endif
#endif

// Define the class, in terms of a parent class
class Sensor_BH1750 : public Sensor_Float {
  public:
    // Define its constructor - used in main.cpp or *.ino
    Sensor_BH1750(const char* const id, const char * const name, const uint8_t addr, TwoWire* wire, const bool retain);
  protected:
    // Define some variables
    const uint8_t addr; // I2C address
    TwoWire* wire; // The I2C interface used
    BH1750 lightmeter; // The data from the library

    // Define any functions we override
    void setup() override;
    float readFloat() override;
    bool validate(const float v) override;
};
#endif // SENSOR_BH1750_H