/*
  Base class for pretty much everything - or should be ! 
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <string>     // std::string, std::stoi
#include "system_base.h"
#include "sensor.h"
#include "actuator.h"
#include "control.h"
#include "misc.h"
#include "system_frugal.h"

System_Base::System_Base(const char * const id, const String name)
: id(id), name(name) { };

// Defaults for routines that can, but often are not, overridden in sub-class.
void System_Base::setup() { };
void System_Base::loop() { }; // Called frequently same as loop() in typical arduino apps
void System_Base::periodically() { }; // Called once for each period - which might be 10 seconds, or seeral hours
void System_Base::infrequently() { }; // Run once each period, but should check timing
//void System_Base::captiveLines(AsyncResponseStream* response) { }; // Called by captive portal for anything to display

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void System_Base::dispatchPath(const String &topicPath, const String &payload) {};
#pragma GCC diagnostic pop
#ifdef SYSTEM_DISCOVERY_SHORT
  void System_Base::discover() {} ; // Default to do nothing
#else
  String System_Base::advertisement() {return String();};
#endif

void System_Base::dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet) {
  if (isSet && (topicSensorId == id)) {
    if (topicTwig == "name") {
      name = payload;
      writeConfigToFS(topicTwig, payload);
    } else {
      // Nothing wrong with no match - it might have been handled in subclass
    }
  }
}
// Basic read configuration - based on the object's "id"
void System_Base::readConfigFromFS() {
  // Note LittleFS should have been setup in frugal_iot constructor so this should not be null
  String path = String("/") + id;
  File dir = frugal_iot.fs_LittleFS->open(path, "r"); // TODO call via System_FS virtual 
  if (dir) {
    readConfigFromFS(dir, nullptr);
  } else {
    frugal_iot.fs_LittleFS->mkdir(path); // There should be a directory, so can write config received over MQTT
  }
}
void System_Base::readConfigFromFS(File dir, const String* leaf) {
  while (true) {
    File entry = dir.openNextFile(); // ESP32 default to "r", ESP8266 takes no argument and always does "r"
    if (!entry) {
      // no more files
      break;
    }
    // Lets presume reading a:  wifi/foo  or b:  sht/temperature or c: sht/temperature/max
    //Serial.print(id); Serial.print("/"); Serial.print(leaf); Serial.print("/"); Serial.print(entry.name());
    const String newleaf = (leaf ? (*leaf + "/") : "") + entry.name();
    Serial.print(id); Serial.print(F("/")); Serial.print(newleaf);
    if (entry.isDirectory()) { // b: entry is directory sht/temperature 
      Serial.println("/");
      readConfigFromFS(entry, &newleaf); 
    } else { // a: id=wifi twiglet=nullptr entry is foo   or c: id=sht twiglet=temperature entry is max
      String payload = entry.readString();
      payload.trim(); // Remove leading/trailing whitespace
      Serial.print("="); Serial.println(payload);
      dispatchTwig(id, newleaf, payload, true);
    }
    entry.close();
  }
}
void System_Base::writeConfigToFS(const String& topicTwig, const String& payload) {
  String path = String("/") + id + "/" + topicTwig;
  frugal_iot.fs_LittleFS->spurt(path, payload);
}

String* System_Base::leaf2path(const char* const leaf) { 
  return frugal_iot.messages->path(id, leaf);
}
// ========== IO - base class for IN and OUT ===== 


IO::IO(const char * const sensorId, const char * const id, const String name, const char* const color, const bool w)
: sensorId(sensorId), id(id), name(name), 
topicTwig(newStringF("%s/%s", sensorId, id)), 
color(color), wireable(w), wiredPath(nullptr) 
{};

IN::IN(const char* sensorId, const char * const id, const String name, const char * const color, const bool w)
: IO(sensorId, id, name, color, w) { };

OUT::OUT(const char* sensorId, const char * const id, const String name, const char * const color, const bool w)
: IO(sensorId, id, name, color, w) { };

void IO::wireTo(String* topicPath) {
  // TODO probably should unsubscribe from previous BUT could be subscribed elsewhere
  wiredPath = topicPath;
  if (topicPath->length() > 0) {
frugal_iot.messages->subscribe(wiredPath);
  }
}
String* IO::path() {
  return frugal_iot.messages->path(topicTwig);
}
void IO::wireTo(IO* io) {
  wireTo(frugal_iot.messages->path(io->topicTwig)); // Subscribe to the twig of the IO
}
void IO::discover() {
  frugal_iot.messages->send(frugal_iot.messages->path(sensorId, "temperature/color"), new String(color), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
}
void IN::discover() {
  IO::discover();
}
void OUT::discover() {
  send();
  IO::discover();
}

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
String IN::StringValue() {
  Serial.println(F("IN::uint16Value should be subclassed"));
  return String();
}

float INuint16::floatValue() {
  return value;
}
bool INuint16::boolValue() {
  return value;
}
uint16_t INuint16::uint16Value() {
  return value;
}
String INuint16::StringValue() {
  return String(value);
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
String INfloat::StringValue() {
  return String(value, (int)width);
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
String INbool::StringValue() {
  return value ? "true" : "false";
}
float INcolor::floatValue() {
  return 0;
}
bool INcolor::boolValue() {
  return 0;
}
uint16_t INcolor::uint16Value() {
  return 0;
}
String INcolor::StringValue() {
  return ""; // TODO-136 convert
}
float INtext::floatValue() {
  return 0; // TODO-136 convert
}
bool INtext::boolValue() {
  return 0; // TODO-136 convert
}
uint16_t INtext::uint16Value() {
  return 0; // TODO-136 convert
}
String INtext::StringValue() {
  return *value;
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
String OUTfloat::StringValue() {
  return String(value, (int)width);
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
String OUTuint16::StringValue() {
  return String(value);
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
String OUTbool::StringValue() {
  return value ? "true" : "false";
}

void IO::setup() {
    // Note topicTwig subscribed to by IN, not by OUT
}

void IN::setup() {
  IO::setup();
  // No longer subscribes since subscribe to node/set/<sensor>/leaf
}
// Options eg: sht/temp set/sht/temp/wire set/sht/temp set/sht/temp/max
bool IN::dispatchLeaf(const String &leaf, const String &p, bool isSet) {
  if (isSet) { // e.g : set/sht/temp/wire set/sht/temp set/sht/temp/max
    if (leaf.startsWith(id) && leaf.endsWith("/wire")) {
      if (!(wiredPath && (p == *wiredPath))) { // if empty, or different
        wireTo(new String(p));
      }
    } // TODO-130 handle other sets, like max - but at INfloat level
  }
  // For now recognizing both xxx/leaf and set/xxx/leaf or setting TODO-130 change UX
  if (leaf == id) { // e.g. sht/temp set/sht/temp
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

bool OUT::dispatchLeaf(const String &leaf, const String &p, bool isSet) {
  if (isSet) { // e.g : set/sht/temp/wire set/sht/temp set/sht/temp/max
    if (leaf.startsWith(id) && leaf.endsWith("/wire")) {
      if (!(wiredPath && (p == *wiredPath))) {
        wiredPath = new String(p);
        sendWired(); // Destination changed, send current value
      }
    }
  }
  // Note intentionally can't set output values directly with e.g. set/sht/temp or sht/temp
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
    Serial.print(" topicTwig="); Serial.print(topicTwig ? topicTwig : "NULL"); 
    Serial.print(" wireable"); Serial.print(wireable);
    if (wireable) {
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
void INtext::debug(const char* const where) {
  IO::debug(where);
  Serial.print(" value="); Serial.println(*value); 
}
void INbool::debug(const char* const where) {
  IO::debug(where);
  Serial.print(F(" value=")); Serial.println(value); 
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
INfloat::INfloat(const char * const sensorId, const char * const id, const String name, float v, uint8_t width, float mn, float mx, char const * const c, const bool w)
  :   IN(sensorId, id, name, c, w), value(v), width(width), min(mn), max(mx) {
}

INfloat::INfloat(const INfloat &other)
: IN(other.sensorId, other.id, other.name, other.color, other.wireable), 
  value(other.value),
  width(other.width)
{ }
INuint16::INuint16(const char * const sensorId, const char * const id, const String name, uint16_t v, uint16_t mn, uint16_t mx, char const * const c, const bool w)
  :   IN(sensorId, id, name, c, w), value(v), min(mn), max(mx) {
}

INuint16::INuint16(const INuint16 &other) 
: IN(other.sensorId, other.id, other.name, other.color, other.wireable) {
  value = other.value;
}

INbool::INbool(const char * const sensorId, const char * const id, const String name, bool value, char const * const color, const bool wireable)
  :   IN(sensorId, id, name, color, wireable), value(value) {
}

INbool::INbool(const INuint16 &other) 
: IN(other.sensorId, other.id, other.name, other.color, other.wireable) {
  value = other.value;
}
INcolor::INcolor(const char * const sensorId, const char * const id, const String name, uint8_t r, uint8_t g, uint8_t b, const bool w)
  :   IN(sensorId, id, name, nullptr, w), r(r), g(g), b(b) {
}
INcolor::INcolor(const char * const sensorId, const char * const id, const String name, const char* color, const bool w)
  :   IN(sensorId, id, name, nullptr, w) {
    convertAndSet(color);
}

INcolor::INcolor(const INcolor &other) 
: IN(other.sensorId, other.id, other.name, other.color, other.wireable) {
  r = other.r;
  g = other.g;
  b = other.b;
}
INtext::INtext(const char * const sensorId, const char * const id, const String name, String* value, char const * const color, const bool wireable)
  :   IN(sensorId, id, name, color, wireable), value(value) {
}

INtext::INtext(const INtext &other) : 
  IN(other.sensorId, other.id, other.name, other.color, other.wireable),
  value(new String(*other.value)) {
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
bool INtext::convertAndSet(const String &payload) {
  if (!(value && payload == *value)) {
    value = new String(payload); // TODO possibly memory leak for old string 
    return true; // Need to rerun calcs
  }
  return false; // nothing changed
}

#ifndef SYSTEM_DISCOVERY_SHORT
// TO_ADD_INxxx TO_ADD_OUTxxx
const char* valueAdvertLineUint16 = "\n  -\n    topic: %s\n    name: %s\n    type: %s\n    min: %d\n    max: %d\n    color: %s\n    display: %s\n    rw: %s\n    group: %s\n    wireable: %d\n    wired: %s";
const char* valueAdvertLineFloat = "\n  -\n    topic: %s\n    name: %s\n    type: %s\n    min: %*f\n    max: %*f\n    color: %s\n    display: %s\n    rw: %s\n    group: %s\n    wireable: %d\n    wired: %s";
const char* valueAdvertLineBool = "\n  -\n    topic: %s\n    name: %s\n    type: %s\n    color: %s\n    display: %s\n    rw: %s\n    group: %s\n    wireable: %d\n    wired: %s";
const char* valueAdvertLineText = "\n  -\n    topic: %s\n    name: %s\n    type: %s\n    color: %s\n    display: %s\n    rw: %s\n    group: %s\n    wireable: %d\n    wired: %s";
const char* valueAdvertLineColor = "\n  -\n    topic: %s\n    name: %s\n    type: %s\n    color: %s\n    display: %s\n    rw: %s\n    group: %s\n    wireable: %d\n    wired: %s";
#endif
// TO_ADD_INxxx
#ifdef SYSTEM_DISCOVERY_SHORT
void INfloat::discover() {
  frugal_iot.messages->send(frugal_iot.messages->path(sensorId, id, "min"), new String(min, (int)width), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  frugal_iot.messages->send(frugal_iot.messages->path(sensorId, id, "max"), new String(max, (int)width), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  IN::discover();
}
void INuint16::discover() {
  frugal_iot.messages->send(frugal_iot.messages->path(sensorId, id, "min"), new String(min), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  frugal_iot.messages->send(frugal_iot.messages->path(sensorId, id, "max"), new String(max), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  IN::discover();
}
// TODO-152A figure out how to send wired
// INbool and INtext, INcolor are fine witb the superclass (just sending color)
#else
String INfloat::advertisement(const String group) {
  // e.g. "\n  -\n    topic: humidity_limit\n    name: Maximum value\n    type: float\n    min: 1\n    max: 100\n    display: slider\n    rw: w"
  return (topicTwig)
  ? StringF(valueAdvertLineFloat, topicTwig->c_str(), name, "float", width, min, width, max, color, "slider", "w", group, wireable ? 1 : 4, (wireable && wiredPath) ? wiredPath->c_str() : "NULL")
  : String();
}
String INuint16::advertisement(const String group) {
  // e.g. "\n  -\n    topic: humidity_limit\n    name: Maximum value\n    type: float\n    min: 1\n    max: 100\n    display: slider\n    rw: w"
  return (topicTwig)
  ? StringF(valueAdvertLineUint16, topicTwig->c_str(), name, "int", min, max, color, "slider", "w", group, wireable, (wireable && wiredPath) ? wiredPath->c_str() : "NULL")
  : String();
}
String INbool::advertisement(const String group) {
  //"\n  -\n    topic: %s\n    name: %s\n    type: %s\n    color: %s\n    display: %s\n    rw: %s\n    group: %s\n    wireable: %d\n    wired: %s";
  return (topicTwig)
  ? StringF(valueAdvertLineBool, topicTwig->c_str(), name, "bool", color, "toggle", "w", group, wireable, (wireable && wiredPath) ? wiredPath->c_str() : "NULL")
  : String();
}
String INtext::advertisement(const String group) {
  // e.g. "\n  -\n    topic: %s\n    name: %s\n    type: %s\n    color: %s\n    display: %s\n    rw: %s\n    group: %s";
  return (topicTwig)
  ? StringF(valueAdvertLineText, topicTwig->c_str(), name, "text", color, "text", "w", group, wireable, (wireable && wiredPath) ? wiredPath->c_str() : "NULL")
  : String();
}
String INcolor::advertisement(const String group) {
  // e.g. "\n  -\n    topic: humidity_limit\n    name: Maximum value\n    type: float\n    min: 1\n    max: 100\n    display: slider\n    rw: w"
  return (topicTwig)
  ? StringF(valueAdvertLineColor, topicTwig->c_str(), name, "color", color, "wheel", "w", group, wireable, (wireable && wiredPath) ? wiredPath->c_str() : "NULL")
  : String();
}
#endif

// ========== OUT for some topic we are potentially sending to ===== 

//OUTfloat::OUTfloat() {};
//OUTbool::OUTbool() {};
// TO_ADD_OUTxxx
OUTbool::OUTbool(const char * const sensorId, const char* const id, const String name, bool v, char const * const color, const bool w)
  :   OUT(sensorId, id, name, color, w), value(v)  {
}
OUTfloat::OUTfloat(const char * const sensorId, const char* const id, const String name,  float v, uint8_t width, float mn, float mx, char const * const color, const bool w)
  :   OUT(sensorId, id, name, color, w), value(v), width(width), min(mn), max(mx) { 
}
OUTuint16::OUTuint16(const char * const sensorId, const char* const id, const String name,  uint16_t v, uint16_t mn, uint16_t mx, char const * const color, const bool w)
  :   OUT(sensorId, id, name, color, w), value(v), min(mn), max(mx) {
}

// OUT::setup() - note OUT does not subscribe to the topic, it only sends on the topic
// OUT::dispatchPath() - wont be called from Control::dispatchPath.

// TO_ADD_OUTxxx
OUTbool::OUTbool(const OUTbool &other) 
: OUT(other.sensorId, other.id, other.name, other.color, other.wireable) {
  value = other.value;
}
OUTfloat::OUTfloat(const OUTfloat &other) 
: OUT(other.sensorId, other.id, other.name, other.color, other.wireable) {
  value = other.value;
  width = other.width;
  min = other.min;
  max = other.max;
}
OUTuint16::OUTuint16(const OUTuint16 &other) 
: OUT(other.sensorId, other.id, other.name, other.color, other.wireable) {
  value = other.value;
}

// TO_ADD_OUTxxx
// Called when either value, or wiredPath changes
void OUTfloat::sendWired() {
  if (wiredPath && wiredPath->length() ) {
    const String* v = new String(value, (int)width);
    frugal_iot.messages->send(wiredPath, v, MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  }
}
void OUTuint16::sendWired() {
  if (wiredPath && wiredPath->length() ) {
    frugal_iot.messages->send(wiredPath, new String(value), MQTT_RETAIN, MQTT_QOS_ATLEAST1); // TODO note defaulting to 1DP which may or may not be appropriate, retain and qos=1 
  }
}
void OUTbool::sendWired() {
  if (wiredPath && wiredPath->length() ) {
    frugal_iot.messages->send(wiredPath, new String(value), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  }
}
// TO-ADD-OUTxxx
void OUTfloat::send() {
    frugal_iot.messages->send(path(), new String(value, (int)width), MQTT_RETAIN, MQTT_QOS_ATLEAST1); 
}
void OUTfloat::set(const float newvalue) {
  #ifdef CONTROL_HUMIDITY_DEBUG
    Serial.print(F("Setting ")); Setting.print(topicTwig); Serial.print(" old="); Serial.print(value); Serial.print(F(" new=")); Serial.println(newvalue);
  #endif
  if (newvalue != value) {
    value = newvalue;
    send();
    sendWired();
  }
}
void OUTuint16::send() {
  frugal_iot.messages->send(path(), new String(value), MQTT_RETAIN, MQTT_QOS_ATLEAST1); 
}
void OUTuint16::set(const uint16_t newvalue) {
  #ifdef CONTROL_HUMIDITY_DEBUG
    Serial.print(F("Setting ")); Setting.print(topicTwig); Serial.print(" old="); Serial.print(value); Serial.print(F(" new=")); Serial.println(newvalue);
  #endif
  if (newvalue != value) {
    value = newvalue;
    frugal_iot.messages->send(path(), new String(value), MQTT_RETAIN, MQTT_QOS_ATLEAST1); 
    send();
    sendWired();
  }
}
void OUTbool::send() {
  frugal_iot.messages->send(path(), new String(value), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
}
void OUTbool::set(const bool newvalue) {
  #ifdef CONTROL_HYSTERISIS_DEBUG
    Serial.print(F("Setting ")); Setting.print(topicTwig); Serial.print(" old="); Serial.print(value); Serial.print(F(" new=")); Serial.println(newvalue);
  #endif
  if (newvalue != value) {
    value = newvalue;
    send();
    sendWired();
  }
}

// TO-ADD-OUTxxx
#ifdef SYSTEM_DISCOVERY_SHORT
void OUTfloat::discover() {
  frugal_iot.messages->send(frugal_iot.messages->path(sensorId, id, "min"), new String(min, (int)width), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  frugal_iot.messages->send(frugal_iot.messages->path(sensorId, id, "max"), new String(min, (int)width), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  OUT::discover();
}
void OUTuint16::discover() {
  frugal_iot.messages->send(frugal_iot.messages->path(sensorId, id, "min"), new String(min), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  frugal_iot.messages->send(frugal_iot.messages->path(sensorId, id, "max"), new String(min), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  OUT::discover();
}
#else
// "\n  -\n    topic: wire_humidity_control_out\n    name: Output to\n    type: topic\n    options: bool\n    display: dropdown\n    rw: w"
String OUTfloat::advertisement(const String group) {
  // "\n  -\n    topic: wire_humidity_control_out\n    name: Output to\n    type: topic\n    options: bool\n    display: dropdown\n    rw: w\n    group: %s"
  // e.g. "\n  -\n    topic: humidity_limit\n    name: Maximum value\n    type: float\n    min: 1\n    max: 100\n    display: slider\n    rw: w\n    group: %s"
  return (topicTwig)
    ? StringF(valueAdvertLineFloat, topicTwig->c_str(), name, "float", width, min, width, max, color, "bar", "r", group, wireable, (wireable && wiredPath) ? wiredPath->c_str() : "NULL")
    : String();
}
// "\n  -\n    topic: wire_humidity_control_out\n    name: Output to\n    type: topic\n    options: bool\n    display: dropdown\n    rw: w"
String OUTuint16::advertisement(const String group) {
  // "\n  -\n    topic: wire_humidity_control_out\n    name: Output to\n    type: topic\n    options: bool\n    display: dropdown\n    rw: w\n    group: %s"
  // e.g. "\n  -\n    topic: humidity_limit\n    name: Maximum value\n    type: float\n    min: 1\n    max: 100\n    display: slider\n    rw: w\n    group: %s"
  return (topicTwig)
    ? StringF(valueAdvertLineUint16, topicTwig->c_str(), name, "int", min, max, color, "bar", "r", group, wireable, (wireable && wiredPath) ? wiredPath->c_str() : "NULL")
    : String();
}
String OUTbool::advertisement(const String group) {
  // "\n  -\n    topic: %s\n    name: %s\n    type: %s\n    color: %s\n    display: %s\n    rw: %s\n    group: %s\n    wireable: %d\n    wired: %s";
  return (topicTwig)
    ? StringF(valueAdvertLineBool, topicTwig->c_str(), name, "bool", color, "toggle", "r", group, wireable, (wireable && wiredPath) ? wiredPath->c_str() : "NULL")
    : String();
    
}
#endif
/* 
//Not used - built for gsheets where followed by a "wireto"
IN* IN::INxxx(IOtype t, const char* sensorId) {
  switch (t) {
    // TO-ADD-INxxx
    case BOOL:
      return new INbool(sensorId, nullptr, nullptr, false, nullptr, true);
    case UINT16:
      return new INuint16(sensorId, nullptr, nullptr, 0, 0, 0, nullptr, true);
    case FLOAT:
      return new INfloat(sensorId, nullptr, nullptr, 0, 0, 0, nullptr, true);
    case COLOR:
      return new INcolor(sensorId, nullptr, nullptr, 0, 0, 0, true);
    default:
      return nullptr;
  }
}
*/

/* 
//Not used - built for gsheets where followed by a "wireto"
IN* IN::INxxx(IOtype t, const char* sensorId) {
  switch (t) {
    // TO-ADD-INxxx
    case BOOL:
      return new INbool(sensorId, nullptr, nullptr, false, nullptr, true);
    case UINT16:
      return new INuint16(sensorId, nullptr, nullptr, 0, 0, 0, nullptr, true);
    case FLOAT:
      return new INfloat(sensorId, nullptr, nullptr, 0, 0, 0, nullptr, true);
    case COLOR:
      return new INcolor(sensorId, nullptr, nullptr, 0, 0, 0, true);
    default:
      return nullptr;
  }
}
*/


// These are mostly to stop the compiler complaining about missing vtables
void shouldBeDefined() { Serial.println(F("something should be defined but is not")); }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#ifdef SYSTEM_DISCOVERY_SHORT
  void OUT::send() { shouldBeDefined(); }
#else
  String IO::advertisement(const String group) { shouldBeDefined(); return String(); }
  String OUT::advertisement(const String group) { shouldBeDefined(); return String(); }
  String IN::advertisement(const String name) { shouldBeDefined(); return String(); }
#endif
float IO::floatValue() { shouldBeDefined(); return 0.0; }
bool IO::dispatchLeaf(const String &twig, const String &p, bool isSet) { shouldBeDefined(); return false; }
float OUT::floatValue() { shouldBeDefined(); return 0.0; }
bool OUT::boolValue() { shouldBeDefined(); return false; }
uint16_t OUT::uint16Value() { shouldBeDefined(); return 0; }
String OUT::StringValue() { shouldBeDefined(); return ""; }
void OUT::sendWired() { shouldBeDefined(); }
bool IN::convertAndSet(const String &payload) { shouldBeDefined(); return false;}
#pragma GCC diagnostic pop
