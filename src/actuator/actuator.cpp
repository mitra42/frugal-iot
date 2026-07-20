/*
  Base class for Actuators
*/

#include <Arduino.h>
#include "actuator/actuator.h"
#include "system/message.h"
#include "misc.h" // for shouldBeDefined

Actuator::Actuator(const char * const id, const char * const name) 
: System_Base(id, name) { } 

void Actuator::setup() {
  // There was a comment on Actuator_digial.cpp about reading config AFTER setting up inputs,
  // its not clear why the order matters especially since input->setup is currently null for all IN subclasses
  for (auto &input : inputs) {
    input->setup();
  }
  readConfigFromFS(); // Reads config (matching one of the Inputs) and passes to our dispatch - should be after inputs and outputs setup (probably)
}

void Actuator::act() { } // Can be do nothing or overridden

void Actuator::dispatch(System_Message &msg) {
    bool changed = false;
    if (msg.module() == id) {
        for (auto &input : inputs) {
            if (input->dispatch(msg)) {
                changed = true;
            }
        }
        System_Base::dispatch(msg);
    }
    if (changed) {
        act();
    }
}
void Actuator::discover() {
  for (auto &input : inputs) {
    input->discover();
  }
}