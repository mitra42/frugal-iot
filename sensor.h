#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "_base.h"

class Sensor : public Frugal_Base {
  public:
    char* topic = NULL; // Topic to send to
    const bool retain = false;
    const int qos = 0; // Default to no guarrantee of delivery

    Sensor();
    static Sensor *first;
    virtual void setup();
    static void setupAll();
    virtual void loop();
    virtual void dispatch(String &topic, String &payload);

}; // Class Sensor

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