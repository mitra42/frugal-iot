#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "_base.h"
#include <forward_list>

class Sensor : public Frugal_Base {
  public:
    char* topic = NULL; // Topic to send to
    const bool retain = false;
    const int qos = 0; // Default to no guarrantee of delivery

    Sensor();
    virtual void setup();
    static void setupAll();
    virtual void loop();
    static void loopAll();
}; // Class Sensor

extern std::forward_list<Sensor*> sensors;

class Sensor_Float : public Sensor {
    float value; 
    Sensor_Float(); 
    virtual float read();
    virtual void set(float newvalue);
    virtual bool changed(float newvalue);
    virtual void act();
};
class Sensor_Uint16 : public Sensor {
    uint16_t value;
    Sensor_Uint16();
    virtual uint16_t read();
    virtual void set(uint16_t newvalue);
    virtual bool changed(uint16_t newvalue);
    virtual void act();
};
#endif // SENSOR_H