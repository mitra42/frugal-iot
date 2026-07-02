#ifndef ACTUATOR_H
#define ACTUATOR_H

#include <Arduino.h>
#include "system_base.h"
#include "system_io.h"

class Actuator : public System_Base {
  public:
  protected:
    // An Actuator has a group of inputs used to control it. Some things (like dispatch) will loop through them.
    std::vector<IN*> inputs; // Vector of inputs
    //Actuator();
    Actuator(const char * const id, const char * const name);
    void discover() override;
    void dispatch(System_Message &msg) override;
    void setup();
    virtual void act();
}; // Class Actuator

#endif // ACTUATOR_H