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

Control::Control(const char * const n, std::vector<IN*> i, std::vector<OUT*> o, std::vector<TCallback> a)
    : Frugal_Base() , name(n), inputs(i), outputs(o), actions(a) { }

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
  for (auto &action : actions) {
    //if (action) {
      action(this); // Actions should call self.outputs[x].set(newvalue); and allow .set to check if changed and send message
    //}
  }
}
void Control::dispatch(const String &topicPath, const String &payload ) {
    bool changed = false;
    String* tl = Mqtt->topicLeaf(topicPath);
    for (auto &input : inputs) {
        if (tl) { // Will be nullptr if no match i.e. path is not local
            // inputs have possible 'control' and therefore dispatchLeaf
            // And inputs also have possible values being set directly
            if (input->dispatchLeaf(*tl, payload)) {
              changed = true; //  // Does not trigger any messages or actions - though data received in response to subscription will.
            }
        }
        // Only inputs are listening to potential topicPaths - i.e. other devices outputs
        if (input->dispatchPath(topicPath, payload)) {
            changed = true; // Changed an input, do the actions
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
void Control::dispatchAll(const String &topicPath, const String &payload) {
  for (Control* c: controls) {
    c->dispatch(topicPath, payload);
  }
}
String Control::advertisementAll() {
  String ad = String();
  for (Control* c: controls) {
    ad += (c->advertisement());
  }
  return ad;
}

std::vector<Control*> controls;


/*
// Example for blinken  - TODO-25 note needs a loop for timing
long unsigned lastblink; // Note local variable in same contex as control_blinken
IN cb_in1 = [1,"blinkspeed",NULL];
OUT cb_out1 = [0, "ledbuiltin", NULL];
IN* cb_ins[3] = [cb_in1, NULL, NULL];
OUT* cb_outs[3] = [cb_out1, NULL, NULL];
Tcallback blink(Control* self) {
  if (lastblink + (self->inputs[0].value*1000)) < millis() { 
    self->outputs[0].set(!!self->outputs[0].value);
    lastblink = millis();
  }
}
Tcallback cb_acts[3] = [xxx, NULL, NULL]
Control* control_blinken = new Control_3xx3x3(cb_ins, cb_outs, cb_acts)
*/
#endif //CONTROL_WANT
