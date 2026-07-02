#ifndef SENSOR_HT_H
#define SENSOR_HT_H

#include "sensor.h"

class Sensor_HT : public Sensor {
  public:
    Sensor_HT(const char* const id, const char * const name, bool retain);
    OUTfloat* temperature;
    OUTfloat* humidity;
  protected:
    void readValidateConvertSet() override; // Combines function of set(read()) since read gets two values from sensor
    void set(const float temp, const float humy);
    void captiveLines(AsyncResponseStream* response);
};


#endif // SENSOR_HT_H