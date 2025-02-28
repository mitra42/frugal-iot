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
bool OUTfloat::boolValue() {
  return value;
}
float OUTbool::floatValue() {
  return value;
}
bool OUTbool::boolValue() {
  return value;
}
String* INstring::stringValue() {
  return value; // Note pointer, not copied
}
float INstring::floatValue() {
  return value->toFloat(); 
}
bool INstring::boolValue() { 
  return *value == "1"; // TODO-121 may need more options
}
void IO::setup(const char * const sensorname) {
    // Note topicLeaf subscribed to by IN, not by OUT
    if (wireable) {
      // wireLeaf = wire_<sensorname>_<topicLeaf>
      wireLeaf = lprintf(strlen(sensorname) + strlen(topicLeaf) + 7, "wire_%s_%s", sensorname, topicLeaf);
      Mqtt->subscribe(wireLeaf);
    }
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
/*
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void IO::set(const float newvalue) {  
#pragma GCC diagnostic pop
  #ifdef CONTROL_DEBUG
    Serial.print(F("IO::set float should be subclassed for ")); Serial.println(name);
  #endif
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void IO::set(const bool newvalue) {  
#pragma GCC diagnostic pop
  #ifdef CONTROL_DEBUG
    Serial.print(F("IO::set bool should be subclassed for ")); Serial.println(name);
  #endif
}
*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
bool IO::dispatchPath(const String &topicPath, const String &payload) {
#pragma GCC diagnostic pop
  #ifdef CONTROL_DEBUG
    Serial.println(F("IO::dispatchPath should be subclassed"));
  #endif
  return false;
}

#ifdef CONTROL_DEBUG
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
void INfloat::debug(const char* const where) {
    IO::debug(where);
    Serial.print(" value="); Serial.println(value); 
}
void INstring::debug(const char* const where) {
  IO::debug(where);
  Serial.print(F(" value=")); Serial.println(*value); 
}
void OUTfloat::debug(const char* const where) {
    IO::debug(where);
    Serial.print(F(" value=")); Serial.println(value); 
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
const char* groupAdvertLine  = "\n  -\n    group: %s\n    name: %s";
const char* valueAdvertLineText = "\n  -\n    topic: %s\n    name: %s\n    type: %s\n    color: %s\n    display: %s\n    rw: %s\n    group: %s";
const char* valueAdvertLineFloat = "\n  -\n    topic: %s\n    name: %s\n    type: %s\n    min: %.1f\n    max: %.1f\n    color: %s\n    display: %s\n    rw: %s\n    group: %s";
const char* valueAdvertLineBool = "\n  -\n    topic: %s\n    name: %s\n    type: %s\n    color: %s\n    display: %s\n    rw: %s\n    group: %s";
const char* wireAdvertLine = "\n  -\n    topic: %s\n    name: %s%s\n    type: %s\n    options: %s\n    display: %s\n    rw: %s\n    group: %s";
String INfloat::advertisement(const char * const group) {
  String ad = String();
  // e.g. "\n  -\n    topic: humidity_limit\n    name: Maximum value\n    type: float\n    min: 1\n    max: 100\n    display: slider\n    rw: w"
  if (topicLeaf) {
    ad += StringF(valueAdvertLineFloat, topicLeaf, name, "float", min, max, color, "slider", "w", group);
  }
  // e.g. "\n  -\n    topic: wire_humidity_control_humiditynow\n    name: Humidity Now\n    type: topic\n    options: float\n    display: dropdown\n    rw: w"
  if (wireLeaf) {
    ad += StringF(wireAdvertLine, wireLeaf, name, " wire from", "topic", "float", "dropdown", "r", group);
  }
  return ad;
}

// INfloat::INstring() {}

INstring::INstring(const char * const n, String* v, const char * const tl, char const * const c, const bool w)
  :   IN(n, tl, c, w), value(v) {
}

INstring::INstring(const INstring &other) : IN(other.name, other.topicLeaf, other.color, other.wireable) {
  value = new String(*(other.value));
}

void INstring::setup(const char * const sensorname) {
  IN::setup(sensorname);
  if (topicLeaf) Mqtt->subscribe(topicLeaf);
}

bool INstring::dispatchLeaf(const String &tl, const String &p) {
  IN::dispatchLeaf(tl, p); // Handle wireLeaf
  if (tl == topicLeaf) {
    if (p != *value) { // TODO pay some attention here, as want to compare strings, not pointers}
      Serial.print(F("XXX110 INstring changed from:"));
      Serial.print(*value);
      Serial.print(F("to "));
      Serial.println(p);
      value = new String(p);
      return true; // Need to rerun calcs
    }
  }
  return false; // nothing changed
}


// Note also has dispatchLeaf via the superclass
// Check incoming message, return true if value changed and should carry out actions
bool INstring::dispatchPath(const String &tp, const String &p) {
  if (wiredPath && (tp == *wiredPath)) {
    if (!value || (p != *value)) { // TODO pay some attention here, as want to compare strings, not pointers}
      value = new String(p);
      return true; // Need to rerun calcs
    }
  }
  return false; 
}
String INstring::advertisement(const char * const group) {
  String ad = String();
  // e.g. "\n  -\n    topic: humidity_limit\n    name: Maximum value\n    type: float\n    min: 1\n    max: 100\n    display: slider\n    rw: w"
  if (topicLeaf) {
    ad += StringF(valueAdvertLineText, topicLeaf, name, "text", color, "slider", "w", group);
  }
  // e.g. "\n  -\n    topic: wire_humidity_control_humiditynow\n    name: Humidity Now\n    type: topic\n    options: float\n    display: dropdown\n    rw: w"
  if (wireLeaf) {
    ad += StringF(wireAdvertLine, wireLeaf, name, " wire from", "topic", "text", "dropdown", "r", group);
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
  #ifdef CONTROL_HUMIDITY_DEBUG
    Serial.print(F("Setting ")); Setting.print(topicLeaf); Serial.print(" old="); Serial.print(value); Serial.print(F(" new=")); Serial.println(newvalue);
  #endif
  if (newvalue != value) {
    value = newvalue;
    Mqtt->messageSend(topicLeaf, value, 1, true, 1 ); // TODO note defaulting to 1DP which may or may not be appropriate, retain and qos=1 
    sendWired();
  }
}
// Called when either value, or wiredPath changes
void OUTbool::sendWired() {
    if (wiredPath) {
      Mqtt->messageSend(*wiredPath, value, true, 1 ); // TODO note defaulting to 1DP which may or may not be appropriate, retain and qos=1 
    }
}
void OUTbool::set(const bool newvalue) {
  #ifdef CONTROL_HUMIDITY_DEBUG
    Serial.print(F("Setting ")); Setting.print(topicLeaf); Serial.print(" old="); Serial.print(value); Serial.print(F(" new=")); Serial.println(newvalue);
  #endif
  if (newvalue != value) {
    value = newvalue;
    Mqtt->messageSend(topicLeaf, value, true, 1 ); // TODO note defaulting to 1DP which may or may not be appropriate, retain and qos=1 
    sendWired();
  }
}
// "\n  -\n    topic: wire_humidity_control_out\n    name: Output to\n    type: topic\n    options: bool\n    display: dropdown\n    rw: w"
String OUTfloat::advertisement(const char * const group) {
  String ad = String();
  // "\n  -\n    topic: wire_humidity_control_out\n    name: Output to\n    type: topic\n    options: bool\n    display: dropdown\n    rw: w\n    group: %s"
  // e.g. "\n  -\n    topic: humidity_limit\n    name: Maximum value\n    type: float\n    min: 1\n    max: 100\n    display: slider\n    rw: w\n    group: %s"
  if (topicLeaf) {
    ad += StringF(valueAdvertLineFloat, topicLeaf, name, "float", min, max, color, "bar", "r", group);
  }
  // e.g. "\n  -\n    topic: wire_humidity_control_humiditynow\n    name: Humidity Now\n    type: topic\n    options: float\n    display: dropdown\n    rw: w\n    group: %s"
  if (wireLeaf) {
    ad += StringF(wireAdvertLine, wireLeaf, name, " wire to", "topic", "float", "dropdown", "w", group);
  }
  return ad;
}
// "\n  -\n    topic: wire_humidity_control_out\n    name: Output to\n    type: topic\n    options: bool\n    display: dropdown\n    rw: w"
String OUTbool::advertisement(const char * const group) {
  String ad = String();

  // e.g. "\n  -\n    topic: humidity_limit\n    name: Maximum value\n    type: float\n    min: 1\n    max: 100\n    display: bar\n    rw: w"
  if (topicLeaf) {
    ad += StringF(valueAdvertLineBool, topicLeaf, name, "bool", color, "toggle", "r", group);
  }
  // e.g. "\n  -\n    topic: wire_humidity_control_out\n    name: Output to\n    type: topic\n    options: bool\n    display: dropdown\n    rw: w"
  if (wireLeaf) {
    ad += StringF(wireAdvertLine, wireLeaf, name, " wire to", "topic", "bool", "dropdown", "w", group);
  }
  return ad;
}

// ==== Control - base class for all controls 

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
    // Duplicate of input code in Control & System_Logger
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
long unsigned lastblink; // Note local variable in same context as control_blinken
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
