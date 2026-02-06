/*
  Base class for Actuators
*/

#include <Arduino.h>
#include "actuator.h"
#include "misc.h" // for shouldBeDefined

Actuator::Actuator(const char * const id, const char * const name) 
: System_Base(id, name) { } 

void Actuator::setup() {
  // There was a comment on Actuator_digial.cpp about reading config AFTER setting up inputs,
  // its not clear why the order matters especially since input->setup is currently null for all IN subclasses
  for (auto &input : inputs) {
    input->setup();
  }
  readConfigFromFS(); // Reads config (matching one of the Inputs) and passes to our dispatchTwig - should be after inputs and outputs setup (probably)
}

void Actuator::act() { } // Can be do nothing or overridden

void Actuator::dispatchTwig(const String &topicActuatorId, const String &topicTwig, const String &payload, bool isSet) {
    bool changed = false;
    if (topicActuatorId == id) { // matches this control
      for (auto &input : inputs) {
        if (input->dispatchLeaf(topicTwig, payload, isSet)) {
          changed = true; // Changed an input, call act()
        }
      }
      System_Base::dispatchTwig(topicActuatorId, topicTwig, payload, isSet);
    }
    if (changed) { 
      act(); // Likely to be subclassed
    }
}
void Actuator::discover() {
  for (auto &input : inputs) {
    input->discover();
  }
}