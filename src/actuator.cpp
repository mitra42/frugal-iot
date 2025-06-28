/*
  Base class for Actuators
*/

#include <Arduino.h>
#include "actuator.h"

Actuator::Actuator(const char * const id, const char * const name) 
: System_Base(id, name) { } 

void Actuator::dispatchTwig(const String &topicActuatorId, const String &topicLeaf, const String &payload, bool isSet) {
  Serial.println("Actuator::dispatchTwig should be subclassed");
}
