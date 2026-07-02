/*
  Base class for pretty much everything - or should be ! 
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <string>     // std::string, std::stoi
#include "system_base.h"
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
// This is here so can do an "add" on a Group that contains System_Base, does nothing on Control or System subclasses, overridden in Sensor and Actuator (via System_SensorActuator)
System_Base* System_Base::powerPins(const uint8_t power3v3, const uint8_t power0v) { return this; }

void System_Base::powerUp(uint8_t pin3v3, uint8_t pin0v) {
  if (pin0v != 0xFF) {
    digitalWrite(pin0v, LOW);
  }
  if (pin3v3 != 0xFF) {
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

System_SensorActuator::System_SensorActuator(const char * const id, const String name) 
: System_Base(id, name) {}

System_SensorActuator* System_SensorActuator::powerPins(const uint8_t power3v3, const uint8_t power0v) {
  power3v3_ = power3v3;
  power0v_ = power0v;
  if (power3v3_ != 0xFF) { 
        pinMode(power3v3_, OUTPUT);
  }
  if (power3v3_ != 0xFF) { 
        pinMode(power0v_, OUTPUT);
  }
  return this; // For chaining
}
// Power management methods
void System_SensorActuator::powerUp() {
  // Default implementation: call System_Base method with stored pins if valid
  if (power3v3_ != 0xFF || power0v_ != 0xFF) {
    System_Base::powerUp(power3v3_, power0v_);
  }
}

void System_SensorActuator::powerDown() {
  // Default implementation: call System_Base method with stored pins if valid
  if (power3v3_ != 0xFF || power0v_ != 0xFF) {
    System_Base::powerDown(power3v3_, power0v_);
  }
}

