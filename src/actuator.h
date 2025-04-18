#ifndef ACTUATOR_H
#define ACTUATOR_H

#include <Arduino.h>
#include "_base.h"
#include <vector>

class Actuator : public Frugal_Base {
  public:
    const char* topicLeaf = NULL; // Topic to receive on
    // unsigned long ms = 10000; // No loops in Actuators currently - uncomment and initialize to zero if have any
    // unsigned long nextLoopTime = 0; // No loops in Actuators currently - uncomment and initialize to zero if have any

    //Actuator();
    Actuator(const char* topicLeaf);
    virtual void setup();
    static void setupAll();
    //virtual void readAndSet();
    //virtual void loop();
    //static void loopAll();
    virtual void inputReceived(const String &payload);
    virtual void dispatchLeaf(const String &topic_msg, const String &payload);
    static void dispatchLeafAll(const String &topicLeaf, const String &payload);
}; // Class Actuator

extern std::vector<Actuator*> actuators;
void Actuator_debug(const char* msg);

#endif // ACTUATOR_H