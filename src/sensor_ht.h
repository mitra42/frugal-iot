#ifndef SENSOR_HT_H
#define SENSOR_HT_H

#include "sensor.h"

class Sensor_HT : public Sensor {
  public:
    OUTfloat* temperature;
    OUTfloat* humidity;
    Sensor_HT(const char* const id, const char * const name, bool retain);
    void readAndSet() override; // Combines function of set(read()) since read gets two values from sensor
    void set(const float temp, const float humy);
    String advertisement() override;
    virtual void dispatchTwig(const String &topicSensorId, const String &topicLeaf, const String &payload, bool isSet) override;
};


#endif // SENSOR_HT_H