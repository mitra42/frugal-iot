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
    Serial.print("XXX25" __FILE__); Serial.print(__LINE__); Serial.print(value); Serial.print(*topicpath); Serial.print("controlleaf="); Serial.println(controlleaf); delay(1000);
};

IO::IO(float v, char const * const tl = nullptr, char const * const cl = nullptr) {
  Serial.print("XXX25" __FILE__); Serial.println(__LINE__); Serial.print(" Converting topicleaf"); Serial.println(tl);
  if (tl) {
    String* tp = Mqtt->topicPath(tl); 
    Serial.print("XXX25" __FILE__); Serial.println(__LINE__); Serial.print(" Converting to"); Serial.println(*tp);
    IO(v, tp, cl);
  } else {
    Serial.print("XXX25 dont think this should happen as should call other constructor __FILE__"); Serial.println(__LINE__);
  }
};
void IO::setup() {
    Serial.print("XXX25" __FILE__); Serial.println(__LINE__); Serial.print("controlleaf="); Serial.println(controlleaf); delay(1000);
    if (controlleaf) Mqtt->subscribe(controlleaf);
    Serial.print("XXX25" __FILE__); Serial.println(__LINE__); delay(1000);
}

void IO::dispatchLeaf(const String &tl, const String &p) {

  if (tl == controlleaf) {
    if (p != *topicpath) {
      topicpath = new String(p);
      Serial.print("XXX25" __FILE__); Serial.println(__LINE__); delay(1000);
      Mqtt->subscribe(*topicpath);
      Serial.print("XXX25" __FILE__); Serial.println(__LINE__); delay(1000);
      // Drop thru and return false;
    }
  }
}

// ========== IN for some topic we are monitoring and the most recent value ===== 

IN::IN() {}

/* Replaced with debug version below TODO-25
IN::IN(float v, String const * tp = nullptr, char const * const cl = nullptr): IO(v,tp,cl) {}
*/
IN::IN(float v, String const * tp = nullptr, char const * const cl = nullptr) {
  Serial.print(__FILE__);Serial.println(__LINE__);
  IO(v, tp, cl);
    Serial.print("IN constructor called with value: "); Serial.print(v);
    if (tp) {
        Serial.print(", topicpath: "); Serial.print(*tp);
    } else {
        Serial.print(", topicpath is nullptr");
    }
    if (cl) {
        Serial.print(", controlleaf: "); Serial.println(cl);
    } else {
        Serial.println(", controlleaf is nullptr");
    }
}
//IN::IN(float v, char const * const tl = nullptr, char const * const cl = nullptr): IO(v,tl,cl) {}
IN::IN(float v, char const * const tl = nullptr, char const * const cl = nullptr) {
  Serial.print(__FILE__); Serial.println(__LINE__);
  IO(v,tl,cl);
  Serial.print(__FILE__); Serial.println(__LINE__);
}

/*
IN::IN(const IN &other) : IO(other.value, other.topicpath, other.controlleaf) {
  Serial.print("IN:IN(other) after __FILE__"); Serial.println(__LINE__);
}
*/

void IN::setup() {
  Serial.print("XXX25" __FILE__); Serial.println(__LINE__); delay(1000);
  Serial.print(value);   Serial.print(*topicpath);   Serial.println(controlleaf);
  Serial.print("XXX25" __FILE__); Serial.println(__LINE__); delay(1000);
  IO::setup();
  Serial.print("XXX25" __FILE__); Serial.println(__LINE__); delay(1000);
  Serial.println(*topicpath);
  Serial.print("XXX25" __FILE__); Serial.println(__LINE__); delay(1000);
  if (topicpath) Mqtt->subscribe(*topicpath);
  Serial.print("XXX25" __FILE__); Serial.println(__LINE__); delay(1000);

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
//OUT::OUT(float v, String const * tp = NULL, char const * const cl = NULL) : IO(v,tp,cl) { }
OUT::OUT(float v, String const * tp = NULL, char const * const cl = NULL) {
  Serial.println(__FILE__); Serial.print(__LINE__);
  IO(v,tp,cl);
  Serial.println(__FILE__); Serial.print(__LINE__);
}

//OUT::OUT(float v, char const * const tl = NULL, char const * const cl = NULL) : {IO(v,tl,cl) { }
OUT::OUT(float v, char const * const tl = NULL, char const * const cl = NULL) { 
  Serial.println(__FILE__); Serial.print(__LINE__);
  IO(v,tl,cl);
  Serial.println(__FILE__); Serial.print(__LINE__);
}
// OUT::setup() - note OUT does not subscribe to the topic, it only sends on the topic
// OUT::dispatchLeaf() - uses IO since wont be incoming topicpath, only a controlleaf

void OUT::set(const float newvalue) {
  if (newvalue != value) {
    value = newvalue;
    Mqtt->messageSend(*topicpath, value, 1, true, 1 ); // TODO note defaulting to 1DP which may or may not be appropriate, retain and qos=1 
  }
}

// ==== Control - base class for all controls 


Control::Control(std::vector<IN> i, std::vector<OUT> o, std::vector<TCallback> a)
    : Frugal_Base(), inputs(i), outputs(o), actions(a) {
      
    //Serial.print("i0value="); Serial.print(i[0].value);
    //Serial.print("i0tp="); Serial.print(*i[0].topicpath);
    //debug("inside constructor");
    // Additional debug statements
    Serial.print("inputs size: "); Serial.println(inputs.size());
    for (size_t idx = 0; idx < inputs.size(); ++idx) {
        Serial.print("IN"); Serial.print(idx); Serial.print(" value: "); Serial.println(inputs[idx].value);
        if (inputs[idx].topicpath) {
            Serial.print("IN"); Serial.print(idx); Serial.print(" topicpath: "); Serial.println(*inputs[idx].topicpath);
        } else {
            Serial.print("IN"); Serial.print(idx); Serial.println(" topicpath is nullptr");
        }
    }

    controls.push_back(this);
}

void Control::debug(const char* const blah) {
  Serial.print("===Control debug==="); Serial.println(blah);
  Serial.print("IN0"); Serial.println(inputs[0].value);
  Serial.println(*inputs[0].topicpath);
  Serial.println("===end Control debug ===");
}

void Control::setup() {
    debug("Control setup");
    Serial.print("XXX25 __FILE__ Control::setup"); Serial.println(__LINE__); delay(1000);
    for (auto &input : inputs) {
        Serial.print("XXX25" __FILE__); Serial.println(__LINE__); delay(1000);
        input.setup();
        Serial.print("XXX25" __FILE__); Serial.println(__LINE__); delay(1000);
    }
    Serial.print("XXX25" __FILE__); Serial.println(__LINE__); delay(1000);
    for (auto &output : outputs) {
        Serial.print("XXX25" __FILE__); Serial.println(__LINE__); delay(1000);
        output.setup();
        Serial.print("XXX25" __FILE__); Serial.println(__LINE__); delay(1000);
    }
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