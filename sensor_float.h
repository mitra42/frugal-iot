#ifndef SENSOR_FLOAT_H
#define SENSOR_FLOAT_H

#include <Arduino.h>
#include "_base.h"
#include "sensor.h"

class Sensor_Float : public Sensor {
  public:
    float value; 
    Sensor_Float(); 
    virtual float read();
    virtual void set(float newvalue);  // TODO-25 prob const
    virtual bool changed(float newvalue); // TODO-25 prob const
    virtual void act();
    virtual void loop();
};
#endif // SENSOR_FLOAT_H