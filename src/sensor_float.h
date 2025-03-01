#ifndef SENSOR_FLOAT_H
#define SENSOR_FLOAT_H

#include <Arduino.h>
#include "_base.h"
#include "sensor.h"

class Sensor_Float : public Sensor {
  public:
    float value; 
    uint8_t width;
    Sensor_Float(const char* topic, uint8_t width, const unsigned long ms, bool retain);
    virtual void readAndSet();
    virtual float read();
    virtual void set(const float newvalue);
    virtual bool changed(const float newvalue);
    virtual void act();
};
#endif // SENSOR_FLOAT_H