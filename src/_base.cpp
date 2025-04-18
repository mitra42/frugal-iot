/*
  Base class for pretty much everything - or should be ! 
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "_base.h"
#include "sensor.h"
#include "actuator.h"
#include "control.h"
#include "misc.h"
#include "system_mqtt.h"

Frugal_Base::Frugal_Base() { }; // Intentionally nothing here

void Frugal_Base::setup() { }; // This will get called if no setup() in subclass 

void Frugal_Base::setupAll() {

  #ifdef SENSOR_WANT // If there are any sensors
    Sensor::setupAll();
  #endif
  #ifdef ACTUATOR_WANT // If there are any actuators
    Actuator::setupAll();
  #endif
  #ifdef CONTROL_WANT
    Control::setupAll();
  #endif
  // TODO-25 calls system.setupAll
}
void Frugal_Base::loop() { }; // This will get called if no loop() in subclass 

void Frugal_Base::loopAll() {
  Sensor::loopAll();
  #ifdef CONTROL_WANT
    Control::loopAll();
  #endif
  //Actuator::loopAll(); // Currently no loops in Actuators
  // TODO-25 call system;
}; // Class FrugalBase

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

# TO_ADD_INxxx 
float INfloat::floatValue() {
  return value;
}
bool INfloat::boolValue() {
  return value;
}
uint16_t INfloat::uint16Value() {
  return value;
}
# TO_ADD_OUTxxx
float OUTfloat::floatValue() {
  return value;
}
bool OUTfloat::boolValue() {
  return value;
}
uint16_t OUTfloat::uint16Value() {
  return value;
}
float OUTuint16::floatValue() {
  return value;
}
bool OUTuint16::boolValue() {
  return value;
}
uint16_t OUTuint16::uint16Value() {
  return value;
}
float OUTbool::floatValue() {
  return value;
}
bool OUTbool::boolValue() {
  return value;
}
uint16_t OUTbool::uint16Value() {
  return value;
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
# TO_ADD_INxxx
void INfloat::debug(const char* const where) {
    IO::debug(where);
    Serial.print(" value="); Serial.println(value); 
}
void INuint16::debug(const char* const where) {
  IO::debug(where);
  Serial.print(" value="); Serial.println(value); 
}
# TO_ADD_OUTxxx
void OUTfloat::debug(const char* const where) {
    IO::debug(where);
    Serial.print(" value="); Serial.println(value); 
}
void OUTbool::debug(const char* const where) {
    IO::debug(where);
    Serial.print(" value="); Serial.println(value); 
}
void OUTuint16::debug(const char* const where) {
  IO::debug(where);
  Serial.print(" value="); Serial.println(value); 
}
#endif

// ========== IN for some topic we are monitoring and the most recent value ===== 

// INfloat::INfloat() {}
# TO_ADD_INxxx
INfloat::INfloat(const char * const n, float v, const char * const tl, float mn, float mx, char const * const c, const bool w)
  :   IN(n, tl, c, w), value(v), min(mn), max(mx) {
}

INfloat::INfloat(const INfloat &other) : IN(other.name, other.topicLeaf, other.color, other.wireable) {
  value = other.value;
}
INuint16::INuint16(const char * const n, uint16_t v, const char * const tl, uint16_t mn, uint16_t mx, char const * const c, const bool w)
  :   IN(n, tl, c, w), value(v), min(mn), max(mx) {
}

INuint16::INuint16(const INuint16 &other) : IN(other.name, other.topicLeaf, other.color, other.wireable) {
  value = other.value;
}

# TO_ADD_INxxx
void INfloat::setup(const char * const sensorname) {
  IN::setup(sensorname);
  if (topicLeaf) Mqtt->subscribe(topicLeaf);
}
void INuint16::setup(const char * const sensorname) {
  IN::setup(sensorname);
  if (topicLeaf) Mqtt->subscribe(topicLeaf);
}
# TO_ADD_INxxx
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
bool INuint16::dispatchLeaf(const String &tl, const String &p) {
  IN::dispatchLeaf(tl, p); // Handle wireLeaf
  if (tl == topicLeaf) {
    uint16_t v = p.toInt();
    if (v != value) {
      value = v;
      return true; // Need to rerun calcs
    }
  }
  return false; // nothing changed
}

# TO_ADD_INxxx
// Note also has dispatchLeaf via the superclass
// Check incoming message, return true if value changed and should call act() on the control
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
bool INuint16::dispatchPath(const String &tp, const String &p) {
  if (wiredPath && (tp == *wiredPath)) {
    uint16_t v = p.toInt();
    if (v != value) {
      value = v;
      return true; // SHould rerun calculations
    }
  }
  return false; 
}
# TO_ADD_INxxx TO_ADD_OUTxxx
const char* valueAdvertLineFloat = "\n  -\n    topic: %s\n    name: %s\n    type: %s\n    min: %.1f\n    max: %.1f\n    color: %s\n    display: %s\n    rw: %s\n    group: %s";
const char* valueAdvertLineBool = "\n  -\n    topic: %s\n    name: %s\n    type: %s\n    color: %s\n    display: %s\n    rw: %s\n    group: %s";
const char* wireAdvertLine = "\n  -\n    topic: %s\n    name: %s%s\n    type: %s\n    options: %s\n    display: %s\n    rw: %s\n    group: %s";
# TO_ADD_INxxx
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
String INuint16::advertisement(const char * const group) {
  String ad = String();
  // e.g. "\n  -\n    topic: humidity_limit\n    name: Maximum value\n    type: float\n    min: 1\n    max: 100\n    display: slider\n    rw: w"
  if (topicLeaf) {
    ad += StringF(valueAdvertLineFloat, topicLeaf, name, "int", min, max, color, "slider", "w", group);
  }
  // e.g. "\n  -\n    topic: wire_humidity_control_humiditynow\n    name: Humidity Now\n    type: topic\n    options: float\n    display: dropdown\n    rw: w"
  if (wireLeaf) {
    ad += StringF(wireAdvertLine, wireLeaf, name, " wire from", "topic", "int", "dropdown", "r", group);
  }
  return ad;
}

// ========== OUT for some topic we are potentially sending to ===== 

//OUTfloat::OUTfloat() {};
//OUTbool::OUTbool() {};
# TO_ADD_OUTxxx
OUTbool::OUTbool(const char * const n, bool v, const char * const tl, char const * const c, const bool w)
  :   OUT(n, tl, c, w), value(v)  {
}
OUTfloat::OUTfloat(const char * const n, float v, const char * const tl, float mn, float mx, char const * const c, const bool w)
  :   OUT(n, tl, c, w), value(v), min(mn), max(mx) {
}
OUTuint16::OUTuint16(const char * const n, uint16_t v, const char * const tl, uint16_t mn, uint16_t mx, char const * const c, const bool w)
  :   OUT(n, tl, c, w), value(v), min(mn), max(mx) {
}

// OUT::setup() - note OUT does not subscribe to the topic, it only sends on the topic
// OUT::dispatchLeaf() - uses IO since wont be incoming topicLeaf or wiredPath, only a wireLeaf
// OUT::dispatchPath() - wont be called from Control::dispatchAll.

# TO_ADD_OUTxxx
OUTbool::OUTbool(const OUTbool &other) : OUT(other.name, other.topicLeaf, other.color, other.wireable) {
  value = other.value;
}
OUTfloat::OUTfloat(const OUTfloat &other) : OUT(other.name, other.topicLeaf, other.color, other.wireable) {
  value = other.value;
}
OUTuint16::OUTuint16(const OUTuint16 &other) : OUT(other.name, other.topicLeaf, other.color, other.wireable) {
  value = other.value;
}

# TO_ADD_OUTxxx
// Called when either value, or wiredPath changes
void OUTfloat::sendWired() {
    if (wiredPath) {
      Mqtt->messageSend(*wiredPath, value, 1, true, 1 ); // TODO note defaulting to 1DP which may or may not be appropriate, retain and qos=1 
    }
}
void OUTuint16::sendWired() {
  if (wiredPath) {
    Mqtt->messageSend(*wiredPath, value, true, 1 ); // TODO note defaulting to 1DP which may or may not be appropriate, retain and qos=1 
  }
}
void OUTbool::sendWired() {
  if (wiredPath) {
    Mqtt->messageSend(*wiredPath, value, true, 1 ); // TODO, retain and qos=1 
  }
}
# TO-ADD-OUTxxx
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
void OUTuint16::set(const uint16_t newvalue) {
  #ifdef CONTROL_HUMIDITY_DEBUG
    Serial.print(F("Setting ")); Setting.print(topicLeaf); Serial.print(" old="); Serial.print(value); Serial.print(F(" new=")); Serial.println(newvalue);
  #endif
  if (newvalue != value) {
    value = newvalue;
    Mqtt->messageSend(topicLeaf, value, true, 1 ); 
    sendWired();
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

#TO-ADD-OUTxxx
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
String OUTuint16::advertisement(const char * const group) {
  String ad = String();
  // "\n  -\n    topic: wire_humidity_control_out\n    name: Output to\n    type: topic\n    options: bool\n    display: dropdown\n    rw: w\n    group: %s"
  // e.g. "\n  -\n    topic: humidity_limit\n    name: Maximum value\n    type: float\n    min: 1\n    max: 100\n    display: slider\n    rw: w\n    group: %s"
  if (topicLeaf) {
    ad += StringF(valueAdvertLineFloat, topicLeaf, name, "int", min, max, color, "bar", "r", group);
  }
  // e.g. "\n  -\n    topic: wire_humidity_control_humiditynow\n    name: Humidity Now\n    type: topic\n    options: float\n    display: dropdown\n    rw: w\n    group: %s"
  if (wireLeaf) {
    ad += StringF(wireAdvertLine, wireLeaf, name, " wire to", "topic", "int", "dropdown", "w", group);
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

