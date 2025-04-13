/*
 *  Logger - log messages to either SPIFSS/LittleFS or SD
 * 
 * Configuration:
 * Required:
 * Optional: 
 */

#include "_settings.h"
#include <Arduino.h>

#ifdef SYSTEM_LOGGER_WANT
#include "system_logger.h"
#include "control.h" // For IN - TODO-110 move IN to Frugal_base
#include "system_time.h" // For system_time.now
#include "misc.h"
#include "system_mqtt.h" // For Mqtt

System_Logger::System_Logger(const char * const n, System_FS* f, const char * const r, const uint8_t strategy, std::vector<IN*> i) : Frugal_Base(), name(n), fs(f), root(r), strategy(strategy), inputs(i) { }

void System_Logger::setup() {
  Serial.println("XXX110 setting up logger");
  fs->setup();
  for (auto &input : inputs) {
    input->setup(name);
  }
}
void System_Logger::setupAll() {
  for (System_Logger* l: loggers) {
    l->setup();
  }
}

// Basis append for logger, there might be other sets of parameters needed = extend as required.
void System_Logger::append(const String &topicPath, const String &payload) {
  #ifdef SYSTEM_LOGGER_DEBUG
    Serial.print("System_Logger::append "); Serial.print(topicPath); Serial.print(" "); Serial.println(payload);
  #endif
  time_t _now = systemTime.now();
  struct tm* tmstruct = localtime(&_now);
  String line;
  String filepath;
  // TODO move to 2007-04-05T14:30Z
  if (strategy & 0x02) { // 0x02 or 0x03
    line = StringF("\"%s\",%04d-%02d-%02d %02d:%02d:%02d,\"%s\"\n", topicPath.c_str(), (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec, payload);  
  } else { // 0x00 or 0x01
    line = StringF("%04d-%02d-%02d %02d:%02d:%02d,\"%s\"\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec, payload);  
  }
  if (strategy & 0x02) {
    if (strategy & 0x01) { // 0x03
      filepath = StringF("%s/%04d-%02d-%02d.csv", root, (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday);
    } else { // 0x02
      filepath = StringF("%s/log.csv", root);
    }
  } else {
    if (strategy & 0x01) { // 0x01 - this is the one that matches the logger format exactly
      filepath = StringF("%s%s/%04d-%02d-%02d.csv", root, topicPath.c_str(), (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday);
    } else { // 0x00
      filepath = StringF("%s%s.csv", root, topicPath.c_str());
    }
  }
  // TODO-110 check errors
  File f = fs->open(filepath,"a"); // Check it will do this recursively creating directories - it might not ! 
  if (!f) { 
    Serial.print(F("Failed to open")); Serial.println(filepath);
  } else {
    uint8_t n = f.print(line); // Will append because opened with "a"
    if (n != line.length()) {
      Serial.print(F("Failed to write to:")); Serial.println(filepath);
    }
    f.close();
  }
}


// Ouput advertisement for control - all of IN and OUTs 
String System_Logger::advertisement() {
  String ad = StringF("\n  -\n    group: %s\n    name: %s", name, name); // Wrap control in a group
  for (auto &input : inputs) {
    ad += (input->advertisement(name));
  }
  return ad;
}
String System_Logger::advertisementAll() {
  String ad = String();
  for (System_Logger* l: loggers) {
    ad += (l->advertisement());
  }
  return ad;
}

void System_Logger::dispatch(const String &topicPath, const String &payload ) {
  // Duplicate of input code in Control & System_Logger
  bool changed = false;
  String* tl = Mqtt->leaf(topicPath);
  for (auto &input : inputs) {
      if (tl) { // Will be nullptr if no match i.e. path is not local
          // inputs have possible 'control' and therefore dispatchLeaf
          // And inputs also have possible values being set directly
          if (input->dispatchLeaf(*tl, payload)) {
            changed = true; // Does not trigger any messages or actions - though data received in response to subscription will.
          }
      }
      // Only inputs are listening to potential topicPaths - i.e. other devices outputs
      if (input->dispatchPath(topicPath, payload)) {
          changed = true; // Changed an input, do the actions
      }
  }
  if (changed) { 
    append(topicPath, payload);
  }
}
// ======= Static functions that work over all Loggers ========
// Note Static
void System_Logger::dispatchAll(const String &topicPath, const String &payload) {
  for (System_Logger* l: loggers) {
    l->dispatch(topicPath, payload);
  }
}

std::vector<System_Logger*> loggers;

#endif //SYSTEM_LOGGER_WANT
