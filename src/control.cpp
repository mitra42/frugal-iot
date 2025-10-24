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

// TODO-ADD-CONTROL

// ==== Control - base class for all controls 

const char* groupAdvertLine  = "\n  -\n    group: %s\n    name: %s";

Control::Control(const char * const id, const char* const name, std::vector<IN*> i, std::vector<OUT*> o)
    : System_Base(id, name), inputs(i), outputs(o) { }

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
    readConfigFromFS(); // Reads config (hostname) and passes to our dispatchTwig - should be after inputs and outputs setup (probably)
  }

void Control::act() {
    // Default is to do nothing - though that will rarely be correct - expect this to be overridden
}
void Control::dispatchTwig(const String &topicControlId, const String &topicTwig, const String &payload, bool isSet) {
    bool changed = false;
    if (topicControlId == id) { // matches this control
      for (auto &input : inputs) {
        if (input->dispatchLeaf(topicTwig, payload, isSet)) {
          changed = true; // Changed an input, call act()
        }
      }
      for (auto &output : outputs) {
        if (output->dispatchLeaf(topicTwig, payload, isSet)) { // Will send value if wiredPath changed
          changed = true; // Shouldnt happen - changing outputs shouldnt cause process, but here for completeness.
        }; 
      }
      System_Base::dispatchTwig(topicControlId, topicTwig, payload, isSet);
    }
    if (changed) { 
      act(); // Likely to be subclassed
    }
}

void Control::dispatchPath(const String &topicPath, const String &payload ) {
    bool changed = false;
    for (auto &input : inputs) {
        // Only inputs are listening to potential topicPaths - i.e. other devices outputs
        if (input->dispatchPath(topicPath, payload)) {
            changed = true; // Changed an input, call act()
        }
    }
    if (changed) { 
      act(); // Likely to be subclassed
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
