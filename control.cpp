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

#ifdef CONTROL_WANT

// TODO-ADD-CONTROL
#if defined(CONTROL_XYZ_DEBUG) || defined(CONTROL_MPQ_DEBUG)
  #define CONTROL_DEBUG
#endif

// ========== IO - base class for IN and OUT ===== 

IO::IO() {}

IO::IO(float v, String const * tp = nullptr, char const * const cl = nullptr): value(v), topicpath(tp), controlleaf(cl) { 
      debug("...IO string constructor ");
};
IO::IO(float v, char const * const tl = nullptr, char const * const cl = nullptr) 
  : IO(v, tl ? Mqtt->topicPath(tl) : nullptr, cl)
{
    debug("...IO char constructor ");
};
void IO::setup() {
    debug("IO setup... ");
    if (controlleaf) Mqtt->subscribe(controlleaf);
    debug("...IO setup ");
}

void IO::dispatchLeaf(const String &tl, const String &p) {

  if (tl == controlleaf) {
    if (p != *topicpath) {
      topicpath = new String(p);
      Mqtt->subscribe(*topicpath);
      // Drop thru and return false;
    }
  }
}
void IO::debug(const char* const where) {
    Serial.print(where); 
    Serial.print(" value="); Serial.print(value); 
    Serial.print(" topicpath="); Serial.print(topicpath ? *topicpath : "NULL"); 
    Serial.print(" controlleaf="); Serial.println(controlleaf ? controlleaf : "NULL");
}

// ========== IN for some topic we are monitoring and the most recent value ===== 

IN::IN() {}

/* Replaced with debug version below TODO-25
IN::IN(float v, String const * tp = nullptr, char const * const cl = nullptr): IO(v,tp,cl) {}
*/
IN::IN(float v, String const * tp = nullptr, char const * const cl = nullptr)
  :   IO(v, tp, cl) {
  debug("...IN string constructor ");
}
//IN::IN(float v, char const * const tl = nullptr, char const * const cl = nullptr): IO(v,tl,cl) {}
IN::IN(float v, char const * const tl = nullptr, char const * const cl = nullptr)
  : IO(v,tl,cl) {
  debug("...IN char constructor ");
}

IN::IN(const IN &other) : IO(other.value, other.topicpath, other.controlleaf) {
  //other.debug("IN copy constructor from:");
  debug("IN copy constructor to:");
}

void IN::setup() {
  debug("IN setup:");
  IO::setup();
  if (topicpath) Mqtt->subscribe(*topicpath);
}

// Note also has dispatchLeaf via the superclass
// Check incoming message, return true if value changed and should carry out actions
bool IN::dispatchPath(const String &tp, const String &p) {
  if (topicpath && (tp == *topicpath)) {
    float v = p.toFloat();
    if (v != value) {
      value = v;
      return true;
    }
  }
  return false; 
}

// ========== OUT for some topic we are potentially sending to ===== 

OUT::OUT() {};

OUT::OUT(float v, String const * tp = NULL, char const * const cl = NULL) : IO(v,tp,cl) {
   debug("OUT string constructor");
}

OUT::OUT(float v, char const * const tl = NULL, char const * const cl = NULL) : IO(v,tl,cl) {
   debug("OUT char constructor");
}
// OUT::setup() - note OUT does not subscribe to the topic, it only sends on the topic
// OUT::dispatchLeaf() - uses IO since wont be incoming topicpath, only a controlleaf

OUT::OUT(const IN &other) : IO(other.value, other.topicpath, other.controlleaf) {
  //other.debug("OUT copy constructor from:");
  debug("OUT copy constructor to:");
}

void OUT::set(const float newvalue) {
  if (newvalue != value) {
    value = newvalue;
    Mqtt->messageSend(*topicpath, value, 1, true, 1 ); // TODO note defaulting to 1DP which may or may not be appropriate, retain and qos=1 
  }
}

// ==== Control - base class for all controls 

Control::Control(std::vector<IN> i, std::vector<OUT> o, std::vector<TCallback> a)
    : Frugal_Base() , inputs(i), outputs(o), actions(a) {
    controls.push_back(this);
}
void Control::debug(const char* const where) {
  Serial.println(where);
  for (auto &input : inputs) {
      input.debug("IN:");
  }
  for (auto &output : outputs) {
      output.debug("OUT:");
  }
}

void Control::setup() {
    debug("Control setup... ");
    for (auto &input : inputs) {
        input.setup();
    }
    for (auto &output : outputs) {
        output.setup();
    }
    debug("...Control setup ");
}

void Control::dispatch(const String &topicpath, const String &payload ) {
    bool changed = false;
    String* tl = Mqtt->topicLeaf(topicpath);
    for (auto &input : inputs) {
        if (tl) { // Will be nullptr if no match
            // Both inputs and outputs have possible 'control' and therefore dispatchLeaf
            input.dispatchLeaf(*tl, payload); //  // Does not trigger any messages or actions - though data received in response to subscription will.
        }
        // Only inputs are listening to potential topicpaths - i.e. other devices outputs
        if (input.dispatchPath(topicpath, payload)) {
            changed = true; // Changed an input, do the actions
        }
    }
    for (auto &output : outputs) {
        if (tl) { // Will be nullptr if no match
            // Both inputs and outputs have possible 'control' and therefore dispatchLeaf
            output.dispatchLeaf(*tl, payload); // TODO-25 Setting an output topic *should* but doesnt yet send a message to new outout.topic
        }
    }
    if (changed) {
        for (auto &action : actions) {
            if (action) {
                action(this); // Actions should call self.outputs[x].set(newvalue); and allow .set to check if changed and send message
            }
        }
    }
}
void Control::setupAll() {
  for (Control* c: controls) {
    c->setup();
  }
}
void Control::dispatchAll(const String &topicpath, const String &payload) {
  for (Control* c: controls) {
    c->dispatch(topicpath, payload);
  }
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
