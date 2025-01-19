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
#if defined(CONTROL_HYSTERISIS_DEBUG) || defined(CONTROL_MPQ_DEBUG)
  #define CONTROL_DEBUG
#endif

// ========== IO - base class for IN and OUT ===== 

// Not sure these are useful, since there are const members that need initializing
//IO::IO() {}
//IOfloat::IOfloat() : IO() {}

IO::IO(const char * const n, const char * const tl, const bool w): name(n), topicLeaf(tl), wireable(w), wiredPath(nullptr) { 
      //debug("...IO string constructor ");
};
IOfloat::IOfloat(const char * const n, float v, const char * const tl, const bool w): IO(n, tl, w), value(v) { 
      //debug("...IOfloat string constructor ");
};
float IOfloat::floatValue() {
  return value;
}
void IO::setup(const char * const sensorname) {
    //debug("IO setup... ");
    // Note topicLeaf subscribed to by IN, not by OUT
    if (wireable) {
              // Allocate a buffer large enough for the concatenated string
        size_t buffer_size = strlen(sensorname) + strlen(name) + 5;
        char* buffer = new char[buffer_size];
        strcpy(buffer, "wire_");
        strcat(buffer, sensorname);
        strcat(buffer, "_");
        strcat(buffer, name);
        wireLeaf = buffer; // Assign buffer to wireLeaf
        Mqtt->subscribe(wireLeaf);
    }
    //debug("...IO setup ");
}

bool IO::dispatchLeaf(const String &tl, const String &p) {
  if (tl == wireLeaf) {
    if (p != *wiredPath) {
      wiredPath = new String(p);
      Mqtt->subscribe(*wiredPath);
    }
  }
  return false; // Should not rerun calculations just because wiredPath changes - but will if/when receive new value
}
void IO::set(const float newvalue) {}

bool IO::dispatchPath(const String &topicPath, const String &payload) {
  #ifdef CONTROL_DEBUG
    Serial.println(F("IO::dispatchPath should be subclassed"));
  #endif
  return false;
}
void IO::debug(const char* const where) {
    Serial.print(where); 
    Serial.print(" topicLeaf="); Serial.print(topicLeaf ? topicLeaf : "NULL"); 
    Serial.print(" wireable"); Serial.print(wireable);
    if (wireable) {
      Serial.print(" wireLeaf"); Serial.print(wireLeaf);
      Serial.print(" wiredPath="); Serial.print(wiredPath ? *wiredPath : "NULL"); 
    }
}
#ifdef CONTROL_DEBUG
void IOfloat::debug(const char* const where) {
    IO::debug(where);
    Serial.print(" value="); Serial.print(value); 
}
#endif

// ========== IN for some topic we are monitoring and the most recent value ===== 

// INfloat::INfloat() {}

/* Replaced with debug version below TODO-25
IN::IN(float v, String const * tp = nullptr, char const * const cl = nullptr): IO(v,tp,cl) {}
*/
INfloat::INfloat(const char * const n, float v, const char * const tl, const bool w)
  :   IOfloat(n, v, tl, w) {
  //debug("...IN string constructor ");
}

INfloat::INfloat(const INfloat &other) : IOfloat(other.name, other.value, other.topicLeaf, other.wireable) {
  //other.debug("IN copy constructor from:");
  //debug("IN copy constructor to:");
}

void INfloat::setup(const char * const sensorname) {
  //debug("IN setup:");
  IOfloat::setup(sensorname);
  if (topicLeaf) Mqtt->subscribe(topicLeaf);
}

bool INfloat::dispatchLeaf(const String &tl, const String &p) {
  IOfloat::dispatchLeaf(tl, p); // Handle wireLeaf
  if (tl == topicLeaf) {
    float v = p.toFloat();
    if (v != value) {
      value = v;
      return true; // Need to rerun calcs
    }
  }
  return false; // nothing changed
}


// Note also has dispatchLeaf via the superclass
// Check incoming message, return true if value changed and should carry out actions
bool INfloat::dispatchPath(const String &tp, const String &p) {
  if (wiredPath && (tp == *wiredPath)) {
    float v = p.toFloat();
    if (v != value) {
      value = v;
      return true; // SHould rerun calculations
    }
  }
  return false; 
}

// ========== OUT for some topic we are potentially sending to ===== 

//OUTfloat::OUTfloat() {};

/* Replaced with debug version below TODO-25
OUTfloat::INfloat(float v, String const * tp = nullptr, char const * const cl = nullptr): IO(v,tp,cl) {}
*/
OUTfloat::OUTfloat(const char * const n, float v, const char * const tl, const bool w)
  :   IOfloat(n, v, tl, w) {
  //debug("...IN char constructor ");
}

// OUT::setup() - note OUT does not subscribe to the topic, it only sends on the topic
// OUT::dispatchLeaf() - uses IO since wont be incoming topicLeaf or wiredPath, only a wireLeaf
// OUT::dispatchPath() - wont be called from Control::dispatchAll.

OUTfloat::OUTfloat(const OUTfloat &other) : IOfloat(other.name, other.value, other.topicLeaf, other.wireable) {
  //other.debug("OUT copy constructor from:");
  //debug("OUT copy constructor to:");
}

void OUTfloat::set(const float newvalue) {
  if (newvalue != value) {
    value = newvalue;
    Mqtt->messageSend(topicLeaf, value, 1, true, 1 ); // TODO note defaulting to 1DP which may or may not be appropriate, retain and qos=1 
    if (wiredPath) {
      Mqtt->messageSend(*wiredPath, value, 1, true, 1 ); // TODO note defaulting to 1DP which may or may not be appropriate, retain and qos=1 
    }
  }
}

// ==== Control - base class for all controls 

Control::Control(const char * const n, std::vector<IO*> i, std::vector<IO*> o, std::vector<TCallback> a)
    : Frugal_Base() , name(n), inputs(i), outputs(o), actions(a) {
    controls.push_back(this);
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
    //debug("Control setup... ");
    for (auto &input : inputs) {
        input->setup(name);
    }
    for (auto &output : outputs) {
        output->setup(name);
    }
    debug("...Control setup ");
}

void Control::dispatch(const String &topicPath, const String &payload ) {
    bool changed = false;
    String* tl = Mqtt->topicLeaf(topicPath);
    for (auto &input : inputs) {
        if (tl) { // Will be nullptr if no match i.e. path is not local
            // Both inputs and outputs have possible 'control' and therefore dispatchLeaf
            input->dispatchLeaf(*tl, payload); //  // Does not trigger any messages or actions - though data received in response to subscription will.
        }
        // Only inputs are listening to potential topicPaths - i.e. other devices outputs
        if (input->dispatchPath(topicPath, payload)) {
            changed = true; // Changed an input, do the actions
        }
    }
    for (auto &output : outputs) {
      if (tl) { // Will be nullptr if no match
        // Both inputs and outputs have possible 'control' and therefore dispatchLeaf
        output->dispatchLeaf(*tl, payload); // TODO-25 Setting an output topic *should* but doesnt yet send a message to new outout.topic
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
// Note Static
void Control::setupAll() {
  for (Control* c: controls) {
    c->setup();
  }
}
// Note Static
void Control::dispatchAll(const String &topicPath, const String &payload) {
  //Serial.println("XXX25 Control::dispatchAll");
  for (Control* c: controls) {
    c->dispatch(topicPath, payload);
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
