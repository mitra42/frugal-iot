#ifndef SENSOR_SOIL_H
#define SENSOR_SOIL_H



#include "sensor_analog.h"

class Sensor_Soil : public Sensor_Analog {
  public: 
    Sensor_Soil(const char* const id, const char * const name, uint8_t pin_init, int offset, float scale, const char* color, bool retain);
  protected:
    bool validRaw(int v);
};

#endif // SENSOR_SOIL_H
