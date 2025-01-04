#ifndef SENSOR_UINT16_H
#define SENSOR_UINT16_H

#include <Arduino.h>
#include "_base.h"
#include "sensor.h"

class Sensor_Uint16 : public Sensor {
  public:
    uint16_t value;
    // If non-zero smoothed by this many bits (newSmoothedValue = oldSmoothedValue - (oldSmoothedValue>>smooth) + (reading))
    // Be careful of overflow - e.g. if 10 bit analog read then max smooth can be is 6 to smooth over 2^6 = 64 readings
    // Note this functionality might get pushed to a superclass
    uint8_t smooth = 0; 

    Sensor_Uint16();
    Sensor_Uint16(const uint8_t smooth, const char* topic, const unsigned long ms);
    virtual uint16_t read();
    virtual void set(uint16_t newvalue);  // TODO-25 prob const
    virtual bool changed(uint16_t newvalue); // TODO-25 prob const
    virtual void act();
    virtual void loop();
};
#endif // SENSOR_UINT16_H
