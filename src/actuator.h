#ifndef ACTUATOR_H
#define ACTUATOR_H

#include <Arduino.h>
#include "system_base.h"

class Actuator : public System_Base {
  public:
    // unsigned long ms = 10000; // No loops in Actuators currently - uncomment and initialize to zero if have any
    // unsigned long nextLoopTime = 0; // No loops in Actuators currently - uncomment and initialize to zero if have any

    //Actuator();
    Actuator(const char * const id, const char * const name);
    virtual void dispatchTwig(const String &topicActuatorId, const String &topicLeaf, const String &payload, bool isSet);
}; // Class Actuator

void Actuator_debug(const char* msg);

#endif // ACTUATOR_H