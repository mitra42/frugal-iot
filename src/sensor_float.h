#ifndef SENSOR_FLOAT_H
#define SENSOR_FLOAT_H

#include <Arduino.h>
#include "system_base.h"
#include "sensor.h"

class Sensor_Float : public Sensor {
  public:
    Sensor_Float(const char* const id, const char * const name, uint8_t width, float min, float max, const char* color, bool retain);
    virtual float readValidateConvert();
  protected:
    OUTfloat* output;
    uint8_t width;
    // TODO-25 need to pass retain & qos down to OUTfloat which needs to use them.
    virtual float readFloat();
    virtual bool validate(float v);
    virtual float convert(float v);
    virtual void set(const float vv);
    void readValidateConvertSet() override;
    void captiveLines(AsyncResponseStream* response) override;
};
#endif // SENSOR_FLOAT_H