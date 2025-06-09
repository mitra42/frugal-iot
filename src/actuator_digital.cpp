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

Actuator_Digital::Actuator_Digital(const char * const id, const char * const name, const uint8_t pin, const char* color)
: Actuator(id, name), 
  pin(pin),
  input(new INbool(id, "on", "On", false, color, false))
{};

void Actuator_Digital::act() {
  digitalWrite(pin, input->value ? HIGH : LOW); // Relay pin on Wemos shield is NOT inverted
}
void Actuator_Digital::set(const bool v) {
  #ifdef ACTUATOR_DIGITAL_DEBUG
    Serial.print(F("\nSetting ")); Serial.print(name); Serial.println(v ? F(" on") : F(" off"));
  #endif
  act();
}

void Actuator_Digital::setup() {
  Actuator::setup();
  // initialize the digital pin as an output.
  pinMode(pin, OUTPUT);
  input->setup(name);
  act(); // Set the digital output to match initial conditions.
}
void Actuator_Digital::dispatchTwig(const String &topicActuatorId, const String &leaf, const String &payload, bool isSet) {
  if (topicActuatorId == id) {
    Serial.print(F("XXX dispatchTwig: ")); Serial.println(id);
    if (input->dispatchLeaf(leaf, payload, isSet)) { // True if changed
      act();
    }
  }
}

String Actuator_Digital::advertisement() {
  return input->advertisement(name); // Note using name of actuator not name of input (which is usually the same)
}

#endif // ACTUATOR_DIGITAL_WANT
