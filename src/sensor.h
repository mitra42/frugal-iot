#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "system_base.h"
#include <vector>

class Sensor : public System_Base {
  public:
    const bool retain = false;
    const int qos = 0; // Default to no guarrantee of delivery
    unsigned long nextLoopTime = 0;

    //Sensor();
    Sensor(const char* id, const char* const name, bool retain);
    virtual void readAndSet();
    void periodically() override;
}; // Class Sensor


#endif // SENSOR_H