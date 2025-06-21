#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "system_base.h"
#include <vector>

class Sensor : public System_Base {
  public:
    const char* id = nullptr; // System readable id
    const char* name = nullptr; // Human readable name of sensor (changeable in UX)
    unsigned long ms = 10000; // 10 second read 
    const bool retain = false;
    const int qos = 0; // Default to no guarrantee of delivery
    unsigned long nextLoopTime = 0;

    //Sensor();
    Sensor(const char* id, const char* const name, const unsigned long ms, bool retain);
    virtual void readAndSet();
    void periodically();
}; // Class Sensor


#endif // SENSOR_H