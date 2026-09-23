/*
  Base class for pretty much everything - or should be ! 
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <string>     // std::string, std::stoi
#include <vector>
#include "system/base.h"
#include "system/frugal.h"
#include "system/interface.h"
#include "misc.h" // pinsPowerUp / pinsPowerDown

System_Base::System_Base(const char * const id, const String name)
: id(id), name(name) { };

// Defaults for routines that can, but often are not, overridden in sub-class.
void System_Base::statusLine(Print* out, const char* leaf, const String& value) {
  out->print(id); out->print('/'); out->print(leaf);
  out->print(' '); out->print(value);
  if (frugal_iot.fs_LittleFS && frugal_iot.fs_LittleFS->exists(String("/") + id + "/" + leaf)) {
    out->print(" *");
  }
  out->print('\n');
}

/* The default every module inherits: its name, which System_Base::dispatch handles and stores.
 *
 * Only when it has been persisted, i.e. someone renamed this module - the compiled-in name is
 * already on the page in the header and is not news. Modules with more state of their own
 * override this and add to it; see System_MQTT for the shape.
 */
void System_Base::statusLines(Print* out, bool full) {
  if (full || (frugal_iot.fs_LittleFS && frugal_iot.fs_LittleFS->exists(String("/") + id + "/name"))) {
    statusLine(out, "name", name);
  }
}

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

/* Read this module's saved config: the flat files named /<id>.<leaf> - see System_FS::configPath.
 *
 * This scans the ROOT for our own prefix rather than opening a directory of our own, because
 * there are no per-module directories any more: each one cost a two-block metadata pair (8KB on
 * a 128KB partition), so creating one per module used the whole filesystem after fifteen modules
 * and nothing could be saved at all.
 *
 * Names are collected before anything is read, and the directory handle closed, because
 * dispatch() below can write to - or delete - files, and modifying a directory while iterating it
 * is not defined. The old code had the same hazard inside one module's directory; here it would
 * be the whole root.
 */
void System_Base::readConfigFromFS() {
  const String prefix = String(id) + ".";
  std::vector<String> names;
  File root = frugal_iot.fs_LittleFS->open("/", "r");
  if (root) {
    while (true) {
      File entry = root.openNextFile();
      if (!entry) {
        break;
      }
      const String entryName = entry.name(); // basename
      if (!entry.isDirectory() && entryName.startsWith(prefix)) {
        names.push_back(entryName);
      }
      entry.close();
    }
    root.close();
  }
  for (const String& entryName: names) {
    const String newleaf = frugal_iot.fs_LittleFS->configDecode(entryName.substring(prefix.length()));
    String payload = frugal_iot.fs_LittleFS->slurp(String("/") + entryName, true);
    payload.trim();
    Serial.print(id); Serial.print(F("/")); Serial.print(newleaf); Serial.print(F("=")); Serial.println(payload);
    System_Message msg(frugal_iot.messages->topicPrefix + "set/" + id + "/" + newleaf, payload, false, 0, MsgFromFS);
    msg.parse();
    dispatch(msg);
  }
}
void System_Base::writeConfigToFS(const String& topicLeaf, const String& payload) {
  frugal_iot.fs_LittleFS->spurt(frugal_iot.fs_LittleFS->configPath(id, topicLeaf), payload);
}
String System_Base::leaf2path(const char* const leaf) { 
  return frugal_iot.messages->path(id, leaf);
}
String System_Base::leaf2path(const String& leaf) { 
  return frugal_iot.messages->path(id, leaf);
}
// The chaining stub - see base.h. Does nothing on a Control or a System module; overridden in
// System_SensorActuator, which both Sensor and Actuator extend.
System_Base* System_Base::powerPins(const uint8_t power3v3, const uint8_t power0v) { return this; }

System_SensorActuator::System_SensorActuator(const char * const id, const String name) 
: System_Base(id, name) {}

/* On a device with a shared bus, the pins go to the BUS - see the note in base.h.
 *
 * The rail feeding an I2C sensor feeds that bus's pull-ups too, so cycling it per sensor would
 * take the bus out from under every other device on it and leave nothing to re-begin() it
 * afterwards. Handing them over means power3v3_/power0v_ stay PIN_NONE here, so this object's
 * own powerUp()/powerDown() become no-ops and the bus does the work at the right moment in
 * System_Power's ordering.
 */
System_SensorActuator* System_SensorActuator::powerPins(const uint8_t power3v3, const uint8_t power0v) {
  System_Interface* bus = powerInterface();
  if (bus) {
    bus->powerPins(power3v3, power0v);
  } else {
    power3v3_ = power3v3;
    power0v_ = power0v;
    if (power3v3_ != PIN_NONE) {
      pinMode(power3v3_, OUTPUT);
    }
    if (power0v_ != PIN_NONE) {
      pinMode(power0v_, OUTPUT);
    }
  }
  return this; // For chaining
}
// Both no-ops when there are no pins, or when powerPins() handed them to a bus instead
void System_SensorActuator::powerUp() {
  pinsPowerUp(power3v3_, power0v_);
}

void System_SensorActuator::powerDown() {
  pinsPowerDown(power3v3_, power0v_); // To high impedance, not driven low - see misc.h
}

