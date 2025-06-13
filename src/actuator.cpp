/*
  Base class for Actuators
*/

#include "_settings.h"  // Settings for what to include etc
#ifdef ACTUATOR_DIGITAL_WANT // defined in _settings.h if subclasses defined
#include <Arduino.h>
#include <vector>
#include "actuator.h"
#include "system_mqtt.h"

Actuator::Actuator(const char * const id, const char * const name) 
: Frugal_Base(id, name) { } 

/* Zero Actuators currently need readAndSet
// TODO_C++_EXPERT - unclear why this is needed, all objects in "actuators" will be subclasses e.g. Actuator_digital each of which has a readAndSet method.
void Actuator::readAndSet() {
  Serial.println("XXX25 Shouldnt be calling Actuator::readAndSet - should be a subclass");
}
*/

void Actuator::dispatchTwig(const String &topicActuatorId, const String &topicLeaf, const String &payload, bool isSet) {
  Serial.println("XXX25 Actuator::dispatchTwig should be subclassed");
}

#endif //ACTUATOR_WANT

