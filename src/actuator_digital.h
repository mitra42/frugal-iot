#ifndef ACTUATOR_DIGITAL_H
#define ACTUATOR_DIGITAL_H

#include "actuator.h" // Superclass

class Actuator_Digital : public Actuator {
  public: 
    Actuator_Digital(const char * const id, const char * const name, const uint8_t pin, const char* color);
  protected:
    uint8_t pin;
    INbool* input;
    virtual void act();
    virtual void set(const bool v);
    virtual void setup() override;
    void discover() override;
    void dispatchTwig(const String &topicActuatorId, const String &leaf, const String &payload, bool isSet);
    void captiveLines(AsyncResponseStream* response);
}; // Class Actuator_Digital

#endif // ACTUATOR_DIGITAL_H