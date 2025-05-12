#ifndef ACTUATOR_DIGITAL_H
#define ACTUATOR_DIGITAL_H

#include "actuator.h" // Superclass

class Actuator_Digital : public Actuator {
  public: 
    uint8_t pin;
    INbool* input;
    
    Actuator_Digital(const char * const id, const char * const name, const uint8_t pin, const char* color);
    virtual void act();
    virtual void set(const bool v);
    virtual void inputReceived(const String &payload);
    virtual void setup();
    String advertisement();
    void dispatchTwig(const String &topicActuatorId, const String &leaf, const String &payload, bool isSet);
}; // Class Actuator_Digital

#endif // ACTUATOR_DIGITAL_H