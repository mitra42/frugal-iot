/*
  Base class for digital actuators 
*/

#include "_settings.h"  // Settings for what to include etc

#ifdef ACTUATOR_DIGITAL_WANT // defined in _settings.h if subclasses defined

#include <Arduino.h>
#include "system_mqtt.h"
#include "system_discovery.h" // 
#include "actuator.h"
#include "actuator_digital.h" // defines ACUATOR_DIGITAL_DEBUG

Actuator_Digital::Actuator_Digital(const uint8_t p, const char* t): Actuator(t), pin(p) {};

void Actuator_Digital::act() {
  digitalWrite(pin, value ? HIGH : LOW); // Relay pin on Wemos shield is NOT inverted
}
void Actuator_Digital::set(const bool v) {
  value = v;
  #ifdef ACTUATOR_DIGITAL_DEBUG
    Serial.print(F("\nSetting ")); Serial.print(*topic); Serial.println(v ? F(" on") : F(" off"));
  #endif
  act();
}
void Actuator_Digital::inputReceived(const String &payload) {
  const bool v = payload.toInt(); // Copied to pin in the loop 
  #ifdef ACTUATOR_DIGITAL_DEBUG
    Serial.print(*topic); Serial.print(F(" received ")); Serial.println(v);
  #endif
  set(v);
}

void Actuator_Digital::setup() {
  Actuator::setup(); //TODO-25 is this right way to call superclass
  // initialize the digital pin as an output.
  pinMode(pin, OUTPUT);
}


#endif // ACTUATOR_DIGITAL_WANT
