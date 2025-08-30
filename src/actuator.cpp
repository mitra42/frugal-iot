/*
  Base class for Actuators
*/

#include <Arduino.h>
#include "actuator.h"

Actuator::Actuator(const char * const id, const char * const name) 
: System_Base(id, name) { } 

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void Actuator::dispatchTwig(const String &topicActuatorId, const String &topicLeaf, const String &payload, bool isSet) {
  Serial.println(F("Actuator::dispatchTwig should be subclassed"));
}
#pragma GCC diagnostic pop

void Actuator::setup() {
  readConfigFromFS(); // Reads config (hostname) and passes to our dispatchTwig - should be after inputs and outputs setup (probably)
}

