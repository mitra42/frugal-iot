#ifndef SENSOR_SOIL_H
#define SENSOR_SOIL_H



#include "sensor_analog.h"

class Sensor_Soil : public Sensor_Analog {
  public: 
    uint16_t map0;
    uint16_t map100;
    Sensor_Soil(const char* const id, const char * const name, const uint16_t map0, const uint16_t map100, const uint8_t pin_init, const uint8_t smooth_init, const char* color, bool retain);
    virtual uint16_t read();
    bool valid(uint16_t newvalue);
};

#endif // SENSOR_SOIL_H
