#ifndef SENSOR_FLOAT_H
#define SENSOR_FLOAT_H

#include <Arduino.h>
#include "_base.h"
#include "sensor.h"

class Sensor_Float : public Sensor {
  public:
    OUTfloat* output;
    uint8_t width;
    // TODO-25 need to pass retain & qos down to OUTfloat which needs to use them.
    Sensor_Float(const char* const id, const char * const name, uint8_t width, float min, float max, const char* color, const unsigned long ms, bool retain);
    virtual void readAndSet();
    virtual float read();
    virtual void set(const float newvalue);
    virtual String advertisement();
    virtual void dispatchTwig(const String &topicSensorId, const String &topicLeaf, const String &payload, bool isSet);
};
#endif // SENSOR_FLOAT_H