#ifndef ACTUATOR_H
#define ACTUATOR_H

#include <Arduino.h>
#include "_base.h"
#include <vector>

class Actuator : public Frugal_Base {
  public:
    const char* id = nullptr; // Name of actuator
    const char* name = nullptr; // Name of actuator
    // unsigned long ms = 10000; // No loops in Actuators currently - uncomment and initialize to zero if have any
    // unsigned long nextLoopTime = 0; // No loops in Actuators currently - uncomment and initialize to zero if have any

    //Actuator();
    Actuator(const char * const id, const char * const name);
    virtual void setup();
    static void setupAll();
    //virtual void readAndSet();
    //virtual void loop();
    //static void loopAll();
    virtual void inputReceived(const String &payload);
    virtual void dispatchTwig(const String &topicActuatorId, const String &topicLeaf, const String &payload, bool isSet);
    static void dispatchTwigAll(const String &topicTwig, const String &payload, bool isSet);
    virtual String advertisement();
    static String advertisementAll();
}; // Class Actuator

extern std::vector<Actuator*> actuators;
void Actuator_debug(const char* msg);

#endif // ACTUATOR_H