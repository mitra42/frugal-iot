#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "_base.h"
#include <vector>

class Sensor : public Frugal_Base {
  public:
    const char* name = NULL; //  name for debugging etc
    unsigned long ms = 10000; // 10 second read 
    const bool retain = false;
    const int qos = 0; // Default to no guarrantee of delivery
    unsigned long nextLoopTime = 0;

    //Sensor();
    Sensor(const char* const name, const unsigned long ms, bool retain);
    virtual void setup();
    static void setupAll();
    virtual void readAndSet();
    virtual void loop();
    static void loopAll();
    virtual String advertisement();
    static String advertisementAll();
}; // Class Sensor

extern std::vector<Sensor*> sensors;

#endif // SENSOR_H