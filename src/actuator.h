#ifndef ACTUATOR_H
#define ACTUATOR_H

#include <Arduino.h>
#include "system_base.h"

class Actuator : public System_Base {
  public:
    //Actuator();
    Actuator(const char * const id, const char * const name);
    virtual void dispatchTwig(const String &topicActuatorId, const String &topicLeaf, const String &payload, bool isSet);
    void setup();
}; // Class Actuator

#endif // ACTUATOR_H