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
        input->setup(name);
    }
    for (auto &output : outputs) {
        output->setup(name);
    }
  }

void Control::act() {
    // Default is to do nothing - though that will rarely be correct
}
void Control::dispatchTwig(const String &topicControlId, const String &leaf, const String &payload, bool isSet) {
    bool changed = false;
    if (topicControlId == id) {
      for (auto &input : inputs) {
        if (input->dispatchLeaf(leaf, payload, isSet)) {
          changed = true; // Changed an input, call act()
        }
      }
      for (auto &output : outputs) {
        if (output->dispatchLeaf(leaf, payload, isSet)) { // Will send value if wiredPath changed
          changed = true; // Shouldnt happen - changing outputs shouldnt cause process, but here for completeness.
        }; 
      }
    }
    if (changed) { 
      act(); // Likely to be subclassed
    }
}

void Control::dispatchPath(const String &topicPath, const String &payload ) {
  //Serial.print("XXX" __FILE__); Serial.print(__LINE__); Serial.print(" Control::dispatchPath: topicPath: "); Serial.println(topicPath);
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

// Ouput advertisement for control - all of IN and OUTs 
String Control::advertisement() {
  String ad = StringF(groupAdvertLine, name, name); // Wrap control in a group
  for (auto &input : inputs) {
    ad += (input->advertisement(name));
  }
  for (auto &output : outputs) {
    ad += (output->advertisement(name));
  }
  return ad;
}
