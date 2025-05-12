#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "_base.h"
#include <vector>

class Sensor : public Frugal_Base {
  public:
    const char* id = nullptr; // System readable id
    const char* name = nullptr; // Human readable name of sensor (changeable in UX)
    unsigned long ms = 10000; // 10 second read 
    const bool retain = false;
    const int qos = 0; // Default to no guarrantee of delivery
    unsigned long nextLoopTime = 0;

    //Sensor();
    Sensor(const char* id, const char* const name, const unsigned long ms, bool retain);
    virtual void setup();
    static void setupAll();
    virtual void readAndSet();
    virtual void loop();
    static void loopAll();
    void inputReceived(const String &payload) {}; // Does nothing in sensors (may change in future)
    virtual String advertisement();
    static String advertisementAll();
    virtual void dispatchTwig(const String &topicSensorId, const String &topicLeaf, const String &payload, bool isSet);
    static void dispatchTwigAll(const String &topicTwig, const String &payload, bool isSet);
}; // Class Sensor

extern std::vector<Sensor*> sensors;

#endif // SENSOR_H