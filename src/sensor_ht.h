#ifndef SENSOR_HT_H
#define SENSOR_HT_H

#include "sensor.h"

class Sensor_HT : public Sensor {
  public:
    const char* topicLeafT; // Sensor::topicLeaf is typically "temperature" topicLeaf2 "humidity"
    const char* topicLeafH; // Sensor::topicLeaf is typically "temperature" topicLeaf2 "humidity"
    float temperature;
    float humidity;
    Sensor_HT(const char* name, const char* topicLeafT, const char* topicLeafH, const unsigned long ms, bool retain);
    virtual void readAndSet(); // Combines function of set(read()) since read gets two values from sensor
};


#endif // SENSOR_HT_H