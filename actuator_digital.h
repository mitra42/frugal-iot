#ifndef ACTUATOR_DIGITAL_H
#define ACTUATOR_DIGITAL_H

// Add new digutal actuators to this statement.  #TO-ADD-ACTUATOR
#if defined(ACTUATOR_RELAY_DEBUG) || defined(ACTUATOR_LEDBUILTIN_DEBUG) // TODO make this generic, but LED almost always wanted
#define ACTUATOR_DIGITAL_DEBUG
#endif 

class Actuator_Digital {
  public: 
    uint8_t pin;
    bool value;
    String topic; //TODO-53 maybe should be &topic
    #ifdef ACTUATOR_DIGITAL_DEBUG
      String *name;
    #endif
    Actuator_Digital(const uint8_t p);
    virtual void act();
    void set(const bool v);
    void messageReceived(const String &topic, const String &payload);
    void setup();
}; // Class Actuator_Digital

#endif // ACTUATOR_DIGITAL_H