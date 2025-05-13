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
#include "system_mqtt.h"
#include "misc.h" //TODO-125 move to wherever watchdog caller moves - maybe base.cpp ? 

#ifdef CONTROL_WANT

// TODO-ADD-CONTROL
#if defined(CONTROL_HYSTERISIS_DEBUG) || defined(CONTROL_MPQ_DEBUG)
  #define CONTROL_DEBUG
#endif


// ==== Control - base class for all controls 

const char* groupAdvertLine  = "\n  -\n    group: %s\n    name: %s";

Control::Control(const char * const id, const char* const name, std::vector<IN*> i, std::vector<OUT*> o)
    : Frugal_Base() , id(id), name(name), inputs(i), outputs(o) { }

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

// Note Static
void Control::setupAll() {
  for (Control* c: controls) {
    Serial.print("Setting up Control:"); Serial.println(c->id);
    c->setup();
  }
}
// Note Static
void Control::loopAll() {
  for (Control* c: controls) {
    c->loop();
  }
}
// Note Static
void Control::dispatchTwigAll(const String &topicTwig, const String &payload, bool isSet) {
  uint8_t slashPos = topicTwig.indexOf('/'); // Find the position of the slash
  if (slashPos != -1) {
    String topicControlId = topicTwig.substring(0, slashPos);       // Extract the part before the slash
    String topicLeaf = topicTwig.substring(slashPos + 1);      // Extract the part after the slash
    for (Control* a: controls) {
      a->dispatchTwig(topicControlId, topicLeaf, payload, isSet);
    }
  } else {
    Serial.println("No slash found in topic: " + topicTwig);
  }
}


void Control::dispatchPathAll(const String &topicPath, const String &payload) {
  for (Control* c: controls) {
    c->dispatchPath(topicPath, payload);
  }
}
// Note Sensor::advertisementAll almost same as Control::advertisementAll so if change one, change the other
String Control::advertisementAll() {
  String ad = String();
  for (Control* c: controls) {
    ad += (c->advertisement());
  }
  return ad;
}

std::vector<Control*> controls;


#endif //CONTROL_WANT
