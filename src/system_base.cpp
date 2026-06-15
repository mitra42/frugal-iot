/*
  Base class for pretty much everything - or should be ! 
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <string>     // std::string, std::stoi
#include "system_base.h"
#include "system_message.h"
#include "sensor.h"
#include "actuator.h"
#include "control.h"
#include "misc.h"
#include "system_frugal.h"

System_Base::System_Base(const char * const id, const String name)
: id(id), name(name) { };

// Defaults for routines that can, but often are not, overridden in sub-class.
void System_Base::setup() { };
void System_Base::setupFailed() { // Call this from setup() if fails
  Serial.print(id); Serial.println(F(" Failed in setup"));
}
void System_Base::loop() { }; // Called frequently same as loop() in typical arduino apps
void System_Base::periodically() { }; // Called once for each period - which might be 10 seconds, or seeral hours
void System_Base::infrequently() { }; // Run once each period, but should check timing
//void System_Base::captiveLines(AsyncResponseStream* response) { }; // Called by captive portal for anything to display

void System_Base::discover() {} ; // Default to do nothing

void System_Base::dispatch(System_Message &msg) {
  if (msg.isSet() && (msg.module() == id)) {
    if (msg.leaf() == "name") {
      if (name != msg.payload) {
        name = msg.payload;
        msg.maybeWriteToFSandEcho();
      }
    }
  }
}

// Basic read configuration - based on the object's "id"
void System_Base::readConfigFromFS() {
  // Note LittleFS should have been setup in frugal_iot constructor so this should not be null
  String path = String("/") + id;
  File dir = frugal_iot.fs_LittleFS->open(path, "r"); // TODO call via System_FS virtual 
  if (dir) {
    readConfigFromFS(dir, nullptr); // closes directory
  } else {
    frugal_iot.fs_LittleFS->mkdir(path); // There should be a directory, so can write config received over MQTT
    Serial.print(F("Creating:")); Serial.println(path);
  }
}
// dir could be sht or one level lower e.g. sht/temperature
void System_Base::readConfigFromFS(File dir, const String* leaf) {
  while (true) {
    File entry = dir.openNextFile(); // ESP32 default to "r", ESP8266 takes no argument and always does "r"
    if (!entry) {
      // no more files
      break;
    }
    // Lets presume reading a:  wifi/foo  or b:  sht/temperature or c: sht/temperature/max
    //Serial.print(id); Serial.print(F("/")); Serial.print(leaf); Serial.print(F("/")); Serial.print(entry.name());
    const String newleaf = (leaf ? (*leaf + "/") : "") + entry.name();
    Serial.print(id); Serial.print(F("/")); Serial.print(newleaf);
    if (entry.isDirectory()) { // b: entry is directory sht/temperature 
      Serial.println(F("/"));
      readConfigFromFS(entry, &newleaf);  // will close entry
    } else { // a: id=wifi twiglet=nullptr entry is foo   or c: id=sht twiglet=temperature entry is max
      String payload = entry.readString();
      entry.close(); // Must close before dispatch which might delete the file
      payload.trim(); // Remove leading/trailing whitespace
      Serial.print(F("=")); Serial.println(payload);
      System_Message msg(frugal_iot.messages->topicPrefix + "set/" + id + "/" + newleaf, payload, false, 0, MsgFromFS);
      msg.parse();
      dispatch(msg);
    }
  }
  dir.close();
}
// Note there is also a IO::writeConfigToFS
void System_Base::writeConfigToFS(const String& topicLeaf, const String& payload) {
  String filepath = String("/") + id + "/" + topicLeaf;
  frugal_iot.fs_LittleFS->spurt(filepath, payload);
}
String System_Base::leaf2path(const char* const leaf) { 
  return frugal_iot.messages->path(id, leaf);
}
String System_Base::leaf2path(const String& leaf) { 
  return frugal_iot.messages->path(id, leaf);
}
void System_Base::powerUp(uint8_t pin3v3, uint8_t pin0v) {
  if (pin0v != 0xFF) {
    pinMode(pin0v, OUTPUT);  // Set pin after reading config as may change
    digitalWrite(pin0v, LOW);
  }
  if (pin3v3 != 0xFF) {
    pinMode(pin3v3, OUTPUT);  // Set pin after reading config as may change
    digitalWrite(pin3v3, HIGH);
  }
}
void System_Base::powerUp() {
  // By default do nothing
}

void System_Base::powerDown(uint8_t pin3v3, uint8_t pin0v) {
  // To power down, go to high impedance input
  if (pin3v3 != 0xFF) {
    pinMode(pin3v3, INPUT); 
  }
  if (pin0v != 0xFF) {
    pinMode(pin0v, INPUT);
  }
}
void System_Base::powerDown() {
  // By default do nothing
}


// ========== IO - base class for IN and OUT ===== 


IO::IO(const char * const sensorId, const char * const id, const String name, const char* const color, const bool w)
: sensorId(sensorId), id(id), name(name), 
topicTwig(StringF("%s/%s", sensorId, id)), 
color(color), default_color(color), wireable(w), wiredPath()
{};

IN::IN(const char* sensorId, const char * const id, const String name, const char * const color, const bool w)
: IO(sensorId, id, name, color, w) { };

OUT::OUT(const char* sensorId, const char * const id, const String name, const char * const color, const bool w)
: IO(sensorId, id, name, color, w) { };

void IO::wireTo(String topicPath) {
  // TODO probably should unsubscribe from previous BUT could be subscribed elsewhere
  wiredPath = topicPath; // can set to empty
  // Intentionally not subscribing out to its wired path, leads to unnecessary loopback
}
void IN::wireTo(String topicPath) {
  // TODO probably should unsubscribe from previous BUT could be subscribed elsewhere
  IO::wireTo(topicPath);
  if (topicPath.length()) {
    frugal_iot.messages->subscribe(wiredPath);
  }
}
String IO::path() {
  return frugal_iot.messages->path(topicTwig);
}
void IO::wireTo(IO* io) {
  wireTo(frugal_iot.messages->path(io->topicTwig)); // Subscribe to the twig of the IO
}
void IO::send() {
  frugal_iot.messages->send(path(), StringValue(), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
}
void IO::discover() {
  send();
  if (wireable) {
    frugal_iot.messages->send(frugal_iot.messages->path(sensorId, id, "wired"), wiredPath, MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  }
  if (color != default_color) { //TODO-213 this is probably not a valid compare, will prob compare ptr not value
    frugal_iot.messages->send(frugal_iot.messages->path(sensorId, id, "color"), String(color), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  }
}

// TO_ADD_INxxx 
float IN::floatValue() {
  shouldBeDefined();
  return 0.0; 
}
bool IN::boolValue() {
  shouldBeDefined();
  return false; 
}
float INuint16::floatValue() {
  return value;
}
bool INuint16::boolValue() {
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
// TODO do we really need all these interconversions, it might be for something we never do 
String INfloat::StringValue() {
  return String(value, (int)width);
}

float INbool::floatValue() {
  return value;
}
bool INbool::boolValue() {
  return value;
}
String INbool::StringValue() {
  return String(value ? "1" : "0");
}
float INcolor::floatValue() {
  return 0;
}
bool INcolor::boolValue() {
  return 0;
}
String INcolor::StringValue() {
  // #RRGGBB
  // Check that always has r, g, b as two digits
  return StringF("#%02x%02x%02x",r,g,b);
}
float INtext::floatValue() {
  return value.toFloat();
}
bool INtext::boolValue() {
  return value == "true" || value.toInt(); // TODO-136 convert
}
String INtext::StringValue() { // Note this isn't a copy, but I think it will get copied if neces
  return value;
}
// TO_ADD_OUTxxx
float OUTfloat::floatValue() {
  return value;
}
bool OUTfloat::boolValue() {
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
String OUTuint16::StringValue() {
  return String(value);
}
float OUTbool::floatValue() {
  return value;
}
bool OUTbool::boolValue() {
  return value;
}
String OUTbool::StringValue() {
  return String(value ? "1" : "0");
  // Not sure why was returning strings like true or false 
  // Since INbool::convert only does toInt it needs to be "1' and "0"
  //return String(value ? "true" : "false");
}
/*float OUTtext::floatValue() {
  return value;
}
bool OUTtext::boolValue() {
  return value;
}
*/
String OUTtext::StringValue() {
  return value;
}

void IO::setup() {
    // Note topicTwig subscribed to by IN, not by OUT
}

void IN::setup() {
  IO::setup();
  // No longer subscribes since subscribe to node/set/<sensor>/leaf
}

// Options eg: sht/temp set/sht/temp/wired set/sht/temp set/sht/temp/max
// maybe rewrite as dispatchParm and override where required
// This is called, looping over all IN by 
bool IN::dispatch(System_Message &msg) {
  if (msg.module() == sensorId) {
    if (msg.isSet()) { // e.g : set/sht/temp/wired Note subclass handles set/sht/temp set/sht/temp/max etc
      if (msg.leaf().startsWith(id) && msg.leaf().endsWith("/wired")) { // Note: this is the "id" of the leaf, not of the sensor
        if (!(msg.payload == wiredPath)) { // if empty, or different
          wireTo(msg.payload);
          msg.maybeWriteToFSandEcho();
          // Does not trigger a changed return
        }
      }
      // Also recognize leaf/value which is how will be written to disk or get problems with directories vs files
      if (msg.leaf() == id || (msg.leaf().startsWith(id) && msg.leaf().endsWith("/value"))) {
        // isSet usually comes from messages and needs echo as (!set) topic
        // !isSet usually comes from FS but also need to send
        msg.maybeWriteToFSandEcho(true); // Append "/value"
        return convertAndSet(msg.payload); // Virtual - depends on type of INxxx
      }
    }
  }
  // Check if topic matches what we are wired to
  if (msg.topicPath == wiredPath) {
    return convertAndSet(msg.payload);
  }
  return false; // Should not rerun calculations just because wiredPath changes - but will if/when receive new value
}

bool OUT::dispatch(System_Message &msg) {
  if (msg.module() == sensorId) {
    if (msg.isSet()) { // e.g : set/sht/temp/wired set/sht/temp set/sht/temp/max
      if (msg.leaf().startsWith(id) && msg.leaf().endsWith("/wired")) { // We are changing the path, not sending a value
        if (!(msg.payload == wiredPath)) {
          wiredPath = msg.payload;
          sendWired(); // Destination changed, send current value
          msg.maybeWriteToFSandEcho();
        }
      }
    }
    // Note intentionally can't set output values directly with e.g. set/sht/temp or sht/temp
    return false; // Should not rerun calculations just because wiredPath changes - but will if/when receive new value
  } else {
    return false;
  }
}

bool INfloat::dispatch(System_Message &msg) {
  if (msg.module() == sensorId) {
    bool dispatched = false;
    if (msg.leaf().startsWith(id)) {
      float v = msg.payload.toFloat();
      if (msg.leaf().endsWith("/max")) {
        max = v;
        dispatched = true;
      } else if (msg.leaf().endsWith("/min")) {
        min = v;
        dispatched = true;
      }
      if (dispatched) {
        msg.maybeWriteToFSandEcho();
        return false; // value didnt change
      }
      // else drop through and dispatch to superclass
    }
    // Catch generic case like color and incoming wired
    return IN::dispatch(msg);
  } else {
    return false;
  }
}
bool OUTfloat::dispatch(System_Message &msg) {
  if (msg.module() == sensorId) {
    bool dispatched = false;
    if (msg.leaf().startsWith(id)) {
      float v = msg.payload.toFloat();
      if (msg.leaf().endsWith("/max")) {
        max = v;
        dispatched = true;
      } else if (msg.leaf().endsWith("/min")) {
        min = v;
        dispatched = true;
      }
      if (dispatched) {
        msg.maybeWriteToFSandEcho();
        return false; // value didnt change
      }
      // else drop through and dispatch to superclass
    }
  }
    // Catch generic case like color
  return OUT::dispatch(msg);
}
bool INuint16::dispatch(System_Message &msg) {
  if (msg.module() == sensorId) {
    bool dispatched = false;
    if (msg.leaf().startsWith(id)) {
      uint16_t v = msg.payload.toInt();
      if (msg.leaf().endsWith("/max")) {
        max = v;
        dispatched = true;
      } else if (msg.leaf().endsWith("/min")) {
        min = v;
        dispatched = true;
      } else if (msg.leaf().endsWith("/cycle")) { //TODO-210 this looks wrong structure though its skipping hte maybeWriteToFSandEcho correctly 
        int16_t newvalue = (int16_t)value + (int16_t)v;
        if (newvalue > (int16_t)max) { newvalue = (int16_t)min; }
        if (newvalue < (int16_t)min) { newvalue = (int16_t)max; }
        value = (uint16_t)newvalue;
        return true; // value changed
      }
      if (dispatched) {
        msg.maybeWriteToFSandEcho();
        return false; // value didnt change (just parameter)
      }
      // else drop through and dispatch to superclass
    }
  }
  // Catch generic case like color
  return IN::dispatch(msg);
}
bool OUTbool::dispatch(System_Message &msg) {
  if (msg.module() == sensorId) {
    bool dispatched = false;
    bool changed = false;
    if (msg.leaf().startsWith(id)) {
      if (msg.leaf().endsWith("/cycle")) {
        set(!value); // will send and sendWired if changed (which it has)
        dispatched = true;
        changed = true;
      }
      if (dispatched) {
        writeConfigToFS(msg.leaf(), msg.payload); // Dont echo since only /cycle and set will do the necessary sends
        return changed;
      }
      // else drop through and dispatch to superclass
    }
  }
  // Catch generic case - currently just /wired  (specifically I don't think it handles /value)
  return OUT::dispatch(msg);
}
bool OUTuint16::dispatch(System_Message &msg) {
  if (msg.module() == sensorId) {
    bool dispatched = false;
    if (msg.leaf().startsWith(id)) {
      uint16_t v = msg.payload.toInt();
      if (msg.leaf().endsWith("/max")) {
        max = v;
        dispatched = true;
      } else if (msg.leaf().endsWith("/min")) {
        min = v;
        dispatched = true;
      } else if (msg.leaf().endsWith("/cycle")) { //TODO move to a function of OUTuint16
        // Complexity here is because this is a uint16 and cycling below min may go negative
        int16_t newvalue = value + (int16_t)v;
        if (newvalue > max) { newvalue = min; }
        if (newvalue < min) { newvalue = max; }
        set((uint16_t)newvalue);
      }
      if (dispatched) {
        msg.maybeWriteToFSandEcho();
        return false; // value didnt change (just parameter)
      }
      // else drop through and dispatch to superclass
    }
  }
  // Catch generic case - currently just /wired (specifically I don't think it handles /value)
  return OUT::dispatch(msg);
}
// OUTtext::dispatch -> OUT::dispatch
/*
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void IO::set(const float newvalue) {  
#pragma GCC diagnostic pop
  #ifdef CONTROL_DEBUG
    shouldBeDefined();
  #endif
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void IO::set(const bool newvalue) {  
#pragma GCC diagnostic pop
  #ifdef CONTROL_DEBUG
    shouldBeDefined();
  #endif
}
*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
bool IO::dispatch(System_Message &msg) {
#pragma GCC diagnostic pop
  shouldBeDefined();
  return false;
}

#ifdef CONTROL_DEBUG
void IO::debug(const char* const where) {
  // Note subclass needs to provide terminating println (usually after a type-dependent value)
    Serial.print(where); 
    Serial.print(F(" topicTwig=")); Serial.print(topicTwig ? topicTwig : "NULL"); 
    Serial.print(F(" wireable")); Serial.print(wireable);
    if (wireable) {
      Serial.print(F(" wiredPath=")); Serial.print(wiredPath); 
    }
}
// TO_ADD_INxxx
void INfloat::debug(const char* const where) {
    IO::debug(where);
    Serial.print(F(" value=")); Serial.println(value); 
}
void INuint16::debug(const char* const where) {
  IO::debug(where);
  Serial.print(F(" value=")); Serial.println(value); 
}
void INcolor::debug(const char* const where) {
  IO::debug(where);
  // TODO-131 should be xx not x for "0" 
  Serial.print(F("r=")); Serial.print(r, HEX); 
  Serial.print(F("g=")); Serial.print(g, HEX); 
  Serial.print(F("b=")); Serial.print(b, HEX); 
}
void INtext::debug(const char* const where) {
  IO::debug(where);
  Serial.print(F(" value=")); Serial.println(value); 
}
void INbool::debug(const char* const where) {
  IO::debug(where);
  Serial.print(F(" value=")); Serial.println(value); 
}
// TO_ADD_OUTxxx
void OUTfloat::debug(const char* const where) {
    IO::debug(where);
    Serial.print(F(" value=")); Serial.println(value); 
}
void OUTbool::debug(const char* const where) {
    IO::debug(where);
    Serial.print(F(" value=")); Serial.println(value); 
}
void OUTuint16::debug(const char* const where) {
  IO::debug(where);
  Serial.print(F(" value=")); Serial.println(value); 
}
void OUTtext::debug(const char* const where) {
  IO::debug(where);
  Serial.print(F(" value=")); Serial.println(value); 
}
#endif

// ========== IN for some topic we are monitoring and the most recent value ===== 

// INfloat::INfloat() {}
// TO_ADD_INxxx
INfloat::INfloat(const char * const sensorId, const char * const id, const String name, float v, uint8_t width, float mn, float mx, float default_min, float default_max, char const * const c, const bool w)
  :   IN(sensorId, id, name, c, w), width(width), value(v), min(mn), max(mx), default_min(default_min), default_max(default_max) {
}
INfloat::INfloat(const char * const sensorId, const char * const id, const String name, float v, uint8_t width, float mn, float mx, char const * const c, const bool w)
  :   INfloat(sensorId, id, name, v, width, mn, mx, mn, mx, c, w) {
}

INfloat::INfloat(const INfloat &other)
: IN(other.sensorId, other.id, other.name, other.color, other.wireable), 
  width(other.width),
  value(other.value),
  min(other.min), max(other.max), default_min(other.default_min), default_max(other.default_max)
{ }
INuint16::INuint16(const char * const sensorId, const char * const id, const String name, uint16_t v, uint16_t mn, uint16_t mx, uint16_t default_min, uint16_t default_max, char const * const c, const bool w)
  :   IN(sensorId, id, name, c, w), value(v), min(mn), max(mx), default_min(default_min), default_max(default_max) {
}
INuint16::INuint16(const char * const sensorId, const char * const id, const String name, uint16_t v, uint16_t mn, uint16_t mx, char const * const c, const bool w)
  : INuint16(sensorId, id, name, v, mn, mx, mn, mx, c, w) {
}
INuint16::INuint16(const INuint16 &other) 
: IN(other.sensorId, other.id, other.name, other.color, other.wireable), default_min(other.default_min), default_max(other.default_max), value(other.value) {
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
    //TODO-130 this will fail as IN needs color set for discovery. 
}
INcolor::INcolor(const char * const sensorId, const char * const id, const String name, const char* const color, const bool w)
  :   IN(sensorId, id, name, color, w) {
    convertAndSet(color);
}

INcolor::INcolor(const INcolor &other) 
: IN(other.sensorId, other.id, other.name, other.color, other.wireable) {
  r = other.r;
  g = other.g;
  b = other.b;
}
INtext::INtext(const char * const sensorId, const char * const id, const String name, String value, char const * const color, const bool wireable)
  :   IN(sensorId, id, name, color, wireable), value(value) {
}

INtext::INtext(const INtext &other) : 
  IN(other.sensorId, other.id, other.name, other.color, other.wireable),
  value(other.value) {
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
  //Serial.print("XXX "); Serial.print(id); Serial.print(F(" converted ")); Serial.print(payload); Serial.print(F(" to ")); Serial.println(v);
  if (v != value) {
    value = v;
    return true; // Need to rerun calcs
  }
  return false; // nothing changed
}
bool INcolor::convertAndSet(const String &p) {
  return convertAndSet(p.c_str());
}
// #RRGGBB (also works on 0xRRGGBB)
bool INcolor::convertAndSet(const char* p1) {
  if (p1[0] == '#') {
    p1 += 1; // Skip # in #x030a1
  } else if (p1[0] == '0' && p1[1] == 'x') {
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
  if (!(payload == value)) {
    value = payload; 
    return true; // Need to rerun calcs
  }
  return false; // nothing changed
}

// TO_ADD_INxxx

void INfloat::discover() {
  if (min != default_min) {
    frugal_iot.messages->send(frugal_iot.messages->path(sensorId, id, "min"), String(min, (int)width), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  }
  if (min != default_max) {
    frugal_iot.messages->send(frugal_iot.messages->path(sensorId, id, "max"), String(max, (int)width), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  }
  IN::discover();
}
void INuint16::discover() {
  if (min != default_min) {
    frugal_iot.messages->send(frugal_iot.messages->path(sensorId, id, "min"), String(min), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  }
  if (min != default_max) {
    frugal_iot.messages->send(frugal_iot.messages->path(sensorId, id, "max"), String(max), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  }
  IN::discover();
}
/* Using base class's which calls class's StringValue
  void INcolor::discover() {
    IN::discover();
  }
*/
// INbool and INtext, INcolor are fine witb the superclass (just sending color)

// ========== OUT for some topic we are potentially sending to ===== 

//OUTfloat::OUTfloat() {};
//OUTbool::OUTbool() {};
// TO_ADD_OUTxxx
OUTbool::OUTbool(const char * const sensorId, const char* const id, const String name, bool v, char const * const color, const bool w)
  :   OUT(sensorId, id, name, color, w), value(v)  {
}
OUTfloat::OUTfloat(const char * const sensorId, const char* const id, const String name,  float v, uint8_t width, float mn, float mx, float default_min, float default_max, char const * const color, const bool w)
  :   OUT(sensorId, id, name, color, w), width(width), min(mn), max(mx), default_min(default_min), default_max(default_max), value(v) {
}
OUTfloat::OUTfloat(const char * const sensorId, const char* const id, const String name,  float v, uint8_t width, float mn, float mx, char const * const color, const bool w)
  :   OUTfloat(sensorId, id, name, v, width, mn, mx, mn, mx, color, w) { 
}
OUTuint16::OUTuint16(const char * const sensorId, const char* const id, const String name,  uint16_t v, uint16_t mn, uint16_t mx, char const * const color, const bool w)
  :   OUT(sensorId, id, name, color, w), value(v), min(mn), max(mx), default_min(mn), default_max(mx) {
}
OUTtext::OUTtext(const char * const sensorId, const char* const id, const String name,  const String v, char const * const color, const bool w)
  :   OUT(sensorId, id, name, color, w), value(v) {
}

// OUT::setup() - note OUT does not subscribe to the topic, it only sends on the topic

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
: OUT(other.sensorId, other.id, other.name, other.color, other.wireable), 
  min(other.min), max(other.max), 
  default_min(other.default_min), default_max(other.default_max),
  value(other.value) {
}

// TO_ADD_OUTxxx
// Called when either value, or wiredPath changes
void OUT::sendWired(bool retain, uint8_t qos) { // defaults to MQTT_RETAIN MQTT_QOS_ATLEAST1
  if (wiredPath.length()) {
    // Three possibilities = normal = send+loop qos=2 & local = loopback; qos=2 & remote = sendRemote
    if (qos == MQTT_QOS_EXACTLY1) {
      // Either loopback or send, but not both
      if (wiredPath.startsWith(frugal_iot.messages->topicPrefix)) {
        frugal_iot.messages->queueLoopback(wiredPath, StringValue());
      } else {
        frugal_iot.messages->sendRemote(wiredPath, StringValue(), retain, qos, 0x00);
      }
    } else {
      frugal_iot.messages->send(wiredPath, StringValue(), MQTT_RETAIN, MQTT_QOS_ATLEAST1); // TODO note defaulting to 1DP which may or may not be appropriate, retain and qos=1 
    }
  }
}
// TO-ADD-OUTxxx
void OUTfloat::set(const float newvalue) {
  if (newvalue != value) {
    value = newvalue;
    send();
    sendWired();
  }
}
void OUTuint16::set(const uint16_t newvalue) {
  if (newvalue != value) {
    value = newvalue;
    send();
    sendWired();
  }
}
void OUTtext::set(const String newvalue) {
  if (newvalue != value) {
    value = newvalue;
    send();
    sendWired();
  }
}
void OUTbool::send() {
  // String(value) will send 0 or 1, while StringValue() would send "true" or "false"
  frugal_iot.messages->send(path(), String(value), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
}
void OUTbool::set(const bool newvalue) {
  if (newvalue != value) {
    value = newvalue;
    send();
    sendWired();
  }
}

// TO-ADD-OUTxxx
void OUTfloat::discover() {
  if (min != default_min) {
    frugal_iot.messages->send(frugal_iot.messages->path(sensorId, id, "min"), String(min, (int)width), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  }
  if (max != default_max) {
    frugal_iot.messages->send(frugal_iot.messages->path(sensorId, id, "max"), String(max, (int)width), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  }
  OUT::discover();
}
void OUTuint16::discover() {
  if (min != default_min) {
    frugal_iot.messages->send(frugal_iot.messages->path(sensorId, id, "min"), String(min), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  }
  if (max != default_max) {
    frugal_iot.messages->send(frugal_iot.messages->path(sensorId, id, "max"), String(max), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  }
  OUT::discover();
}
// OUTtext::discover -> OUT::discover

// These are mostly to stop the compiler complaining about missing vtables
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
float IO::floatValue() { shouldBeDefined(); return 0.0; }
float OUT::floatValue() { shouldBeDefined(); return 0.0; }
bool OUT::boolValue() { shouldBeDefined(); return false; }
String IO::StringValue() { shouldBeDefined(); return String(); }
bool IN::convertAndSet(const String &payload) { shouldBeDefined(); return false;}
#pragma GCC diagnostic pop
