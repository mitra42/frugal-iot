#ifndef SENSOR_FLOAT_H
#define SENSOR_FLOAT_H

#include <Arduino.h>
#include "system_base.h"
#include "sensor.h"

class Sensor_Float : public Sensor {
  public:
    OUTfloat* output;
    uint8_t width;
    // TODO-25 need to pass retain & qos down to OUTfloat which needs to use them.
    Sensor_Float(const char* const id, const char * const name, uint8_t width, float min, float max, const char* color, bool retain);
    void readAndSet() override;
    virtual float read();
    void set(const float newvalue);
    String advertisement() override;
    virtual void dispatchTwig(const String &topicSensorId, const String &topicLeaf, const String &payload, bool isSet) override;
};
#endif // SENSOR_FLOAT_H