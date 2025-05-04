/*
  Base class for pretty much everything - or should be ! 
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <string>     // std::string, std::stoi
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


IO::IO(const char * const sensorId, const char * const id, const char * const name, const char* const color, const bool w)
: sensorId(sensorId), id(id), name(name), topicLeaf(lprintf(strlen(sensorId)+strlen(id)+2, "%s/%s", sensorId, id)), color(color), wireable(w), wireLeaf(nullptr), wiredPath(nullptr) 
{};
// TODO-130 deprecated below
IO::IO(const char * const n, const char * const tl, const char* const color, const bool w)
: sensorId(nullptr), name(n), topicLeaf(tl), color(color), wireable(w), wireLeaf(nullptr), wiredPath(nullptr) { };

IN::IN(const char * const n, const char * const tl, const char * const color, const bool w): IO(n, tl, color, w) { };

OUT::OUT(const char* sensorId, const char * const id, const char * const name, const char * const color, const bool w)
: IO(sensorId, id, name, color, w) { };

// TO_ADD_INxxx 
float IN::floatValue() {
  Serial.println(F("IN::floatValue should be subclassed"));
  return 0.0; 
}
bool IN::boolValue() {
  Serial.println(F("IN::boolValue should be subclassed"));
  return false; 
}
uint16_t IN::uint16Value() {
  Serial.println(F("IN::uint16Value should be subclassed"));
  return 0;
}
float INfloat::floatValue() {
  return value;
}
bool INfloat::boolValue() {
  return value;
}
uint16_t INfloat::uint16Value() {
  return value;
}
float INbool::floatValue() {
  return value;
}
bool INbool::boolValue() {
  return value;
}
uint16_t INbool::uint16Value() {
  return value;
}

// TO_ADD_OUTxxx
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

void IN::setup(const char * const sensorname) {
  IO::setup(sensorname);
  if (topicLeaf) Mqtt->subscribe(topicLeaf);
}
bool IN::dispatchLeaf(const String &tl, const String &p) {
  if (tl == wireLeaf) {
    if (!(wiredPath && (p == *wiredPath))) {
      wiredPath = new String(p);
      Mqtt->subscribe(*wiredPath);
    }
  }
  if (tl == topicLeaf) {
    return convertAndSet(p); // Virtual - depends on type of INxxx
  }
  return false; // Should not rerun calculations just because wiredPath changes - but will if/when receive new value
}
// Check incoming message, return true if value changed and should call act() on the control
bool IN::dispatchPath(const String &tp, const String &p) {
  if (wiredPath && (tp == *wiredPath)) {
    return convertAndSet(p);
  }
  return false; // nothing changed
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
// TO_ADD_INxxx
void INfloat::debug(const char* const where) {
    IO::debug(where);
    Serial.print(" value="); Serial.println(value); 
}
void INuint16::debug(const char* const where) {
  IO::debug(where);
  Serial.print(" value="); Serial.println(value); 
}
void INcolor::debug(const char* const where) {
  IO::debug(where);
  // TODO-131 should be xx not x for "0" 
  Serial.print("r="); Serial.print(r, HEX); 
  Serial.print("g="); Serial.print(g, HEX); 
  Serial.print("b="); Serial.print(b, HEX); 
}
void INbool::debug(const char* const where) {
  IO::debug(where);
  Serial.print(" value="); Serial.println(value); 
}
// TO_ADD_OUTxxx
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
// TO_ADD_INxxx
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

INbool::INbool(const char * const name, bool value, const char * const leaf, char const * const color, const bool wireable)
  :   IN(name, leaf, color, wireable), value(value) {
}

INbool::INbool(const INuint16 &other) 
: IN(other.name, other.topicLeaf, other.color, other.wireable) {
  value = other.value;
}
INcolor::INcolor(const char * const n, uint8_t r, uint8_t g, uint8_t b, const char * const tl, const bool w)
  :   IN(n, tl, nullptr, w), r(r), g(g), b(b) {
}
INcolor::INcolor(const char * const n, const char* color, const char * const tl, const bool w)
  :   IN(n, tl, nullptr, w) {
    convertAndSet(color);
}

INcolor::INcolor(const INcolor &other) : IN(other.name, other.topicLeaf, other.color, other.wireable) {
  r = other.r;
  g = other.g;
  b = other.b;
}

// TO_ADD_INxxx
bool INfloat::convertAndSet(const String &p) {
  float v = p.toFloat();
  if (v != value) {
    value = v;
    return true; // Need to rerun calcs
  }
  return false; // nothing changed
}
bool INuint16::convertAndSet(const String &p) {
  uint16_t v = p.toInt();
  if (v != value) {
    value = v;
    return true; // Need to rerun calcs
  }
  return false; // nothing changed
}
bool INbool::convertAndSet(const String &payload) {
  const bool v = payload.toInt();
  if (v != value) {
    value = v;
    return true; // Need to rerun calcs
  }
  return false; // nothing changed
}
bool INcolor::convertAndSet(const String &p) {
  return convertAndSet(p.c_str());
}
bool INcolor::convertAndSet(const char* p1) {
  if (p1[0] == '0' && p1[1] == 'x') {
    p1 += 2; // Skip 0x
  }
  uint32_t rgb = strtoul(p1, nullptr, 16);
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (rgb >> 8) & 0xFF;
  uint8_t b = rgb & 0xFF;
  if ((r != this->r) || (g != this->g) || (b != this->b)) {
    this->r = r;
    this->g = g;
    this->b = b;
    return true; // Need to rerun calcs
  }
  return false; // nothing changed
}
// TO_ADD_INxxx TO_ADD_OUTxxx
const char* valueAdvertLineFloat = "\n  -\n    topic: %s\n    name: %s\n    type: %s\n    min: %.1f\n    max: %.1f\n    color: %s\n    display: %s\n    rw: %s\n    group: %s";
const char* valueAdvertLineBool = "\n  -\n    topic: %s\n    name: %s\n    type: %s\n    color: %s\n    display: %s\n    rw: %s\n    group: %s";
const char* valueAdvertLineColor = "\n  -\n    topic: %s\n    name: %s\n    type: %s\n    color: %s\n    display: %s\n    rw: %s\n    group: %s";
const char* wireAdvertLine = "\n  -\n    topic: %s\n    name: %s%s\n    type: %s\n    options: %s\n    display: %s\n    rw: %s\n    group: %s";
// TO_ADD_INxxx
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
String INbool::advertisement(const char * const group) {
  String ad = String();
  // e.g. "\n  -\n    topic: %s\n    name: %s\n    type: %s\n    color: %s\n    display: %s\n    rw: %s\n    group: %s";
  if (topicLeaf) {
    ad += StringF(valueAdvertLineBool, topicLeaf, name, "bool", color, "toggle", "w", group);
  }
  // e.g. "\n  -\n    topic: wire_humidity_control_humiditynow\n    name: Humidity Now\n    type: topic\n    options: float\n    display: dropdown\n    rw: w"
  if (wireLeaf) {
    ad += StringF(wireAdvertLine, wireLeaf, name, " wire from", "topic", "bool", "dropdown", "r", group);
  }
  return ad;
}
String INcolor::advertisement(const char * const group) {
  String ad = String();
  // e.g. "\n  -\n    topic: humidity_limit\n    name: Maximum value\n    type: float\n    min: 1\n    max: 100\n    display: slider\n    rw: w"
  if (topicLeaf) {
    ad += StringF(valueAdvertLineColor, topicLeaf, name, "color", color, "wheel", "w", group);
  }
  if (wireLeaf) {
    ad += StringF(wireAdvertLine, wireLeaf, name, " wire from", "topic", "color", "dropdown", "r", group);
  }
  return ad;
}

// ========== OUT for some topic we are potentially sending to ===== 

//OUTfloat::OUTfloat() {};
//OUTbool::OUTbool() {};
// TO_ADD_OUTxxx
OUTbool::OUTbool(const char * const sensorId, const char* const id, const char * const name, bool v, char const * const color, const bool w)
  :   OUT(sensorId, id, name, color, w), value(v)  {
}
OUTfloat::OUTfloat(const char * const sensorId, const char* const id, const char * const name,  float v, uint8_t width, float mn, float mx, char const * const color, const bool w)
  :   OUT(sensorId, id, name, color, w), value(v), width(width), min(mn), max(mx) { 
}
OUTuint16::OUTuint16(const char * const sensorId, const char* const id, const char * const name,  uint16_t v, uint16_t mn, uint16_t mx, char const * const color, const bool w)
  :   OUT(sensorId, id, name, color, w), value(v), min(mn), max(mx) {
}

// OUT::setup() - note OUT does not subscribe to the topic, it only sends on the topic
// OUT::dispatchLeaf() - uses IO since wont be incoming topicLeaf or wiredPath, only a wireLeaf
// OUT::dispatchPath() - wont be called from Control::dispatchAll.

// TO_ADD_OUTxxx
OUTbool::OUTbool(const OUTbool &other) : OUT(other.name, other.topicLeaf, other.color, other.wireable) {
  value = other.value;
}
OUTfloat::OUTfloat(const OUTfloat &other) : OUT(other.name, other.topicLeaf, other.color, other.wireable) {
  value = other.value;
  width = other.width;
  min = other.min;
  max = other.max;
}
OUTuint16::OUTuint16(const OUTuint16 &other) : OUT(other.name, other.topicLeaf, other.color, other.wireable) {
  value = other.value;
}

// TO_ADD_OUTxxx
// Called when either value, or wiredPath changes
void OUTfloat::sendWired() {
    if (wiredPath) {
      Mqtt->messageSend(*wiredPath, value, width, true, 1 ); // TODO note retain and qos=1 
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
// TO-ADD-OUTxxx
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

// TO-ADD-OUTxxx
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

void shouldBeDefined() { Serial.println(F("something should be defined but is not")); }
String IO::advertisement(const char * const group) { shouldBeDefined(); return String(); }
float IO::floatValue() { shouldBeDefined(); return 0.0; }
bool IO::dispatchLeaf(const String &tl, const String &p) { shouldBeDefined(); return false; }
String OUT::advertisement(const char * const group) { shouldBeDefined(); return String(); }
float OUT::floatValue() { shouldBeDefined(); return 0.0; }
bool OUT::boolValue() { shouldBeDefined(); return false; }
uint16_t OUT::uint16Value() { shouldBeDefined(); return 0; }
void OUT::sendWired() { shouldBeDefined(); }