#ifndef ACTUATOR_H
#define ACTUATOR_H

#include <Arduino.h>
#include "system_base.h"

class Actuator : public System_Base {
  public:
  protected:
    std::vector<IN*> inputs; // Vector of inputs
    //Actuator();
    Actuator(const char * const id, const char * const name);
    void discover() override;
    virtual void dispatchTwig(const String &topicActuatorId, const String &topicLeaf, const String &payload, bool isSet) override;
    void setup();
    virtual void act();
}; // Class Actuator

#endif // ACTUATOR_H