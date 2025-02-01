#ifndef ACTUATOR_DIGITAL_H
#define ACTUATOR_DIGITAL_H

#include "actuator.h" // Superclass

class Actuator_Digital : Actuator {
  public: 
    uint8_t pin;
    bool value;
    Actuator_Digital(const uint8_t p, const char* topic);
    virtual void act();
    virtual void set(const bool v);
    virtual void inputReceived(const String &payload);
    virtual void setup();
}; // Class Actuator_Digital

#endif // ACTUATOR_DIGITAL_H