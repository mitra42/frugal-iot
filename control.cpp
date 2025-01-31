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
#include "misc.h"

#ifdef CONTROL_WANT

// TODO-ADD-CONTROL
#if defined(CONTROL_HYSTERISIS_DEBUG) || defined(CONTROL_MPQ_DEBUG)
  #define CONTROL_DEBUG
#endif

// ========== IO - base class for IN and OUT ===== 

// Not sure these are useful, since there are const members that need initializing
//IO::IO() {}
//IN::IN() {}
//OUT::OUT() {}

IO::IO(const char * const n, const char * const tl, const char* const c, const bool w): name(n), topicLeaf(tl), color(c), wireable(w), wireLeaf(nullptr), wiredPath(nullptr) { 
      //debug("...IO string constructor ");
};
IN::IN(const char * const n, const char * const tl, const char * const c, const bool w): IO(n, tl, c, w) { };

OUT::OUT(const char * const n, const char * const tl, const char * const c, const bool w): IO(n, tl, c, w) { };

float INfloat::floatValue() {
  return value;
}
bool INfloat::boolValue() {
  return value;
}
float OUTfloat::floatValue() {
  return value;
}
bool OUTbool::boolValue() {
  return value;
}
float OUTbool::floatValue() {
  return value;
}
bool OUTfloat::boolValue() {
  return value;
}

void IO::setup(const char * const sensorname) {
    //debug("IO setup... ");
    // Note topicLeaf subscribed to by IN, not by OUT
    if (wireable) {
      // wireLeaf = wire_<sensorname>_<name>
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

bool IN::dispatchLeaf(const String &tl, const String &p) {
  if (tl == wireLeaf) {
    if (!(wiredPath && (p == *wiredPath))) {
      wiredPath = new String(p);
      Mqtt->subscribe(*wiredPath);
    }
  }
  return false; // Should not rerun calculations just because wiredPath changes - but will if/when receive new value
}
bool OUT::dispatchLeaf(const String &tl, const String &p) {
  if (tl == wireLeaf) {
    if (!(wiredPath && (p == *wiredPath))) {
      wiredPath = new String(p);
      sendWired(); // Destination changed, send current value
    }
  }
  return false; // Should not rerun calculations just because wiredPath changes - but will if/when receive new value
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void IO::set(const float newvalue) {  
#pragma GCC diagnostic pop
  #ifdef CONTROL_DEBUG
    Serial.println(F("IO::set should be subclassed"));
  #endif
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
bool IO::dispatchPath(const String &topicPath, const String &payload) {
#pragma GCC diagnostic pop
  #ifdef CONTROL_DEBUG
    Serial.println(F("IO::dispatchPath should be subclassed"));
  #endif
  return false;
}

void IO::debug(const char* const where) {
  // Note subclass needs to provide terminating println (usually after a type-dependent value)
    Serial.print(where); 
    Serial.print(" topicLeaf="); Serial.print(topicLeaf ? topicLeaf : "NULL"); 
    Serial.print(" wireable"); Serial.print(wireable);
    if (wireable) {
      Serial.print(" wireLeaf"); Serial.print(wireLeaf);
      Serial.print(" wiredPath="); Serial.print(wiredPath ? *wiredPath : "NULL"); 
    }
}
#ifdef CONTROL_DEBUG
void INfloat::debug(const char* const where) {
    IO::debug(where);
    Serial.print(" value="); Serial.println(value); 
}
void OUTfloat::debug(const char* const where) {
    IO::debug(where);
    Serial.print(" value="); Serial.println(value); 
}
void OUTbool::debug(const char* const where) {
    IO::debug(where);
    Serial.print(" value="); Serial.println(value); 
}
#endif

// ========== IN for some topic we are monitoring and the most recent value ===== 

// INfloat::INfloat() {}

INfloat::INfloat(const char * const n, float v, const char * const tl, float mn, float mx, char const * const c, const bool w)
  :   IN(n, tl, c, w), value(v), min(mn), max(mx) {
}

INfloat::INfloat(const INfloat &other) : IN(other.name, other.topicLeaf, other.color, other.wireable) {
  value = other.value;
}

void INfloat::setup(const char * const sensorname) {
  //debug("IN setup:");
  IN::setup(sensorname);
  if (topicLeaf) Mqtt->subscribe(topicLeaf);
}

bool INfloat::dispatchLeaf(const String &tl, const String &p) {
  IN::dispatchLeaf(tl, p); // Handle wireLeaf
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

const char* valueAdvertLineFloat = "\n  -\n    topic: %s\n    name: %s\n    type: %s\n    min: %s\n    max: %s\n    color: %s\n    display: %s\n    rw: %s";
const char* valueAdvertLineBool = "\n  -\n    topic: %s\n    name: %s\n    type: %s\n    color: %s\n    display: %s\n    rw: %s";
const char* wireAdvertLine = "\n  -\n    topic: %s\n    name: %s%s\n    type: %s\n    options: %s\n    display: %s\n    rw: %s";
String *INfloat::advertisement() {
  String* ad = new String();
  // e.g. "\n  -\n    topic: humidity_limit\n    name: Maximum value\n    type: float\n    min: 1\n    max: 100\n    display: slider\n    rw: w"
  if (topicLeaf) {
    *ad += StringF(valueAdvertLineFloat, topicLeaf, name, "float", min, max, color, "slider", "w");
  }
  // e.g. "\n  -\n    topic: wire_humidity_control_humiditynow\n    name: Humidity Now\n    type: topic\n    options: float\n    display: dropdown\n    rw: w"
  if (wireLeaf) {
    *ad += StringF(wireAdvertLine, wireLeaf, name, " wire from", "topic", "float", "dropdown", "w");
  }
  return ad;
}

// ========== OUT for some topic we are potentially sending to ===== 

//OUTfloat::OUTfloat() {};
//OUTbool::OUTbool() {};

OUTbool::OUTbool(const char * const n, bool v, const char * const tl, char const * const c, const bool w)
  :   OUT(n, tl, c, w), value(v)  {
}
OUTfloat::OUTfloat(const char * const n, float v, const char * const tl, float mn, float mx, char const * const c, const bool w)
  :   OUT(n, tl, c, w), value(v), min(mn), max(mx) {
}

// OUT::setup() - note OUT does not subscribe to the topic, it only sends on the topic
// OUT::dispatchLeaf() - uses IO since wont be incoming topicLeaf or wiredPath, only a wireLeaf
// OUT::dispatchPath() - wont be called from Control::dispatchAll.

OUTbool::OUTbool(const OUTbool &other) : OUT(other.name, other.topicLeaf, other.color, other.wireable) {
  value = other.value;
}
OUTfloat::OUTfloat(const OUTfloat &other) : OUT(other.name, other.topicLeaf, other.color, other.wireable) {
  value = other.value;
}

// Called when either value, or wiredPath changes
void OUTfloat::sendWired() {
    if (wiredPath) {
      Mqtt->messageSend(*wiredPath, value, 1, true, 1 ); // TODO note defaulting to 1DP which may or may not be appropriate, retain and qos=1 
    }
}
void OUTfloat::set(const float newvalue) {
  if (newvalue != value) {
    value = newvalue;
    Mqtt->messageSend(topicLeaf, value, 1, true, 1 ); // TODO note defaulting to 1DP which may or may not be appropriate, retain and qos=1 
    sendWired();
  }
}
// Called when either value, or wiredPath changes
void OUTbool::sendWired() {
    if (wiredPath) {
      Mqtt->messageSend(*wiredPath, value, 1, true, 1 ); // TODO note defaulting to 1DP which may or may not be appropriate, retain and qos=1 
    }
}
void OUTbool::set(const bool newvalue) {
  if (newvalue != value) {
    value = newvalue;
    Mqtt->messageSend(topicLeaf, value, 1, true, 1 ); // TODO note defaulting to 1DP which may or may not be appropriate, retain and qos=1 
    sendWired();
  }
}
// "\n  -\n    topic: wire_humidity_control_out\n    name: Output to\n    type: topic\n    options: bool\n    display: dropdown\n    rw: w"
String *OUTfloat::advertisement() {
  String* ad = new String("");
  // "\n  -\n    topic: wire_humidity_control_out\n    name: Output to\n    type: topic\n    options: bool\n    display: dropdown\n    rw: w"
  // e.g. "\n  -\n    topic: humidity_limit\n    name: Maximum value\n    type: float\n    min: 1\n    max: 100\n    display: slider\n    rw: w"
  if (topicLeaf) {
    *ad += StringF(valueAdvertLineFloat, topicLeaf, name, "float", min, max, color, "bar", "r");
  }
  // e.g. "\n  -\n    topic: wire_humidity_control_humiditynow\n    name: Humidity Now\n    type: topic\n    options: float\n    display: dropdown\n    rw: w"
  if (wireLeaf) {
    *ad += StringF(wireAdvertLine, wireLeaf, name, " wire to", "topic", "float", "dropdown", "w");
  }
  return ad;
}
// "\n  -\n    topic: wire_humidity_control_out\n    name: Output to\n    type: topic\n    options: bool\n    display: dropdown\n    rw: w"
String *OUTbool::advertisement() {
  String* ad = new String("");

  // e.g. "\n  -\n    topic: humidity_limit\n    name: Maximum value\n    type: float\n    min: 1\n    max: 100\n    display: bar\n    rw: w"
  if (topicLeaf) {
    *ad += StringF(valueAdvertLineBool, topicLeaf, name, "bool", color, "toggle", "r");
  }
  // e.g. "\n  -\n    topic: wire_humidity_control_out\n    name: Output to\n    type: topic\n    options: bool\n    display: dropdown\n    rw: w"
  if (wireLeaf) {
    *ad += StringF(wireAdvertLine, wireLeaf, name, " wire to", "topic", "bool", "dropdown", "w");
  }
  return ad;
}

// ==== Control - base class for all controls 

Control::Control(const char * const n, std::vector<IN*> i, std::vector<OUT*> o, std::vector<TCallback> a)
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
    //debug("...Control setup ");
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
      if (tl) { // Will be nullptr if no match i.e. path is not local
        // Both inputs and outputs have possible 'control' and therefore dispatchLeaf
        output->dispatchLeaf(*tl, payload); // Will send value if wiredPath changed
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
// Ouput advertisement for control - all of IN and OUTs 
String* Control::advertisement() {
  String* ad = new String();
  for (auto &input : inputs) {
    *ad += *(input->advertisement());
  }
  for (auto &output : outputs) {
    *ad += *(output->advertisement());
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
String* Control::advertisementAll() {
  String* ad = new String();
  for (Control* c: controls) {
    *ad += *(c->advertisement());
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
