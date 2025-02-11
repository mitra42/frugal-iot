#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "_base.h"
#include <vector>

class Sensor : public Frugal_Base {
  public:
    const char* topic = NULL; // Topic to send to (leaf)
    const bool retain = false;
    const int qos = 0; // Default to no guarrantee of delivery
    unsigned long ms = 10000; // 10 second read 
    unsigned long nextLoopTime = 0;

    //Sensor();
    Sensor(const char* const topic, const unsigned long ms);
    virtual void setup();
    static void setupAll();
    virtual void readAndSet();
    virtual void loop();
    static void loopAll();
}; // Class Sensor

extern std::vector<Sensor*> sensors;
void Sensor_debug(const char* const msg);

#endif // SENSOR_H