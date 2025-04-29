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

Control::Control(const char * const n, std::vector<IN*> i, std::vector<OUT*> o)
    : Frugal_Base() , name(n), inputs(i), outputs(o) { }

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
void Control::dispatch(const String &topicPath, const String &payload ) {
    bool changed = false;
    String* tl = Mqtt->leaf(topicPath);
    for (auto &input : inputs) {
        if (tl) { // Will be nullptr if no match i.e. path is not local
            // inputs have possible 'control' and therefore dispatchLeaf
            // And inputs also have possible values being set directly
            if (input->dispatchLeaf(*tl, payload)) {
              changed = true; // Changed an input, call act()
            }
        }
        // Only inputs are listening to potential topicPaths - i.e. other devices outputs
        if (input->dispatchPath(topicPath, payload)) {
            changed = true; // Changed an input, call act()
        }
    }
    for (auto &output : outputs) {
      if (tl) { // Will be nullptr if no match i.e. path is not local
        // outputs have possible 'control' and therefore dispatchLeaf
        output->dispatchLeaf(*tl, payload); // Will send value if wiredPath changed
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
void Control::dispatchPathAll(const String &topicPath, const String &payload) {
  for (Control* c: controls) {
    c->dispatch(topicPath, payload);
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
