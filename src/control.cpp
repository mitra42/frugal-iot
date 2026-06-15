/*
 * Generic base class for controls
 *
 * It makes some assumptions - e.g. max 3 float inputs, which if wrong may require refactoring. 
 * 
 */

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <vector>
#include "control.h"
#include "misc.h"
#include "system_message.h"

// TODO-ADD-CONTROL

// ==== Control - base class for all controls 

const char* groupAdvertLine  = "\n  -\n    group: %s\n    name: %s";

Control::Control(const char * const id, const char* const name, std::vector<IN*> i, std::vector<OUT*> o)
  : System_Base(id, name), inputs(i), outputs(o) 
  { 
  }

#ifdef CONTROL_DEBUG
void Control::debug(const char* const where) {
  Serial.println(where);
  for (auto &input : inputs) {
      input->debug("IN:");
  }
  for (auto &output : outputs) {
      output->debug("OUT:");
  }
}
#endif

void Control::setup() {
  for (auto &input : inputs) {
      input->setup();
  }
  for (auto &output : outputs) {
      output->setup();
  }
  readConfigFromFS(); // Reads config (inputs or outputs) and passes to our dispatch - should be after inputs and outputs setup (probably)
}

void Control::act() {
    // Default is to do nothing - though that will rarely be correct - expect this to be overridden
}
//TODO-120 come back and review this

void Control::dispatch(System_Message &msg) {
    bool changed = false;
    if (msg.module() == id) {
      for (auto &output : outputs) {
        if (output->dispatch(msg)) {
          changed = true;
        }
      }
      System_Base::dispatch(msg);
    }
    // Do these even if not module=id because maybe matches wired input
    for (auto &input : inputs) {
        if (input->dispatch(msg)) {
            changed = true;
        }
    }
    if (changed) {
      act();
    }
}

void Control::discover() {
  for (auto &input : inputs) {
    input->discover();
  }
  for (auto &output : outputs) {
    output->discover();
  }
}
