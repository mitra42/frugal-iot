#ifndef SENSOR_HT_H
#define SENSOR_HT_H

#include "sensor.h"

class Sensor_HT : public Sensor {
  public:
    const char* topic2; // Sensor::topic is typically "temperature" topic2 "humidity"
    float temperature;
    float humidity;
    Sensor_HT(const char* topic, const char* topic2, const unsigned long ms, bool retain);
    virtual void readAndSet(); // Combines function of set(read()) since read gets two values from sensor
};


#endif // SENSOR_HT_H