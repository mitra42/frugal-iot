/*
 *  Logger - log messages to either SPIFSS/LittleFS or SD
 * 
 * Configuration:
 * Required:
 * Optional: 
 * Status:  This works for its current requirement, i.e. 
 *   Can be programatically or via the UI to any message
 *   Can write to SPIFFS or SD in a number of formats
 * 
 * TODO: See https://github.com/mitra42/frugal-iot/issues/110
 */

#include "_settings.h"
#include <Arduino.h>

#ifdef CONTROL_LOGGERFS_WANT
#include "control_logger_fs.h"
#include "control_logger.h"
#include "misc.h"
#include "frugal_iot.h"

Control_LoggerFS::Control_LoggerFS(const char * const name, System_FS* f, const char * const r, const uint8_t strategy, std::vector<IN*> i) 
: Control_Logger("loggerfs",name, i), 
  fs(f), root(r), strategy(strategy), needAppend(false)
{}

void Control_LoggerFS::setup() {
  fs->setup();
  Control::setup(); // Call base class setup
}

// Basis append for logger, there might be other sets of parameters needed = extend as required.
void Control_LoggerFS::append(const String &topicPath, const String &payload) {
  #ifdef CONTROL_LOGGERFS_DEBUG
    Serial.print("Control_Logger::append "); Serial.print(topicPath); Serial.print(" "); Serial.println(payload);
  #endif
  #ifdef SYSTEM_TIME_WANT
    time_t _now = frugal_iot.time->now(); 
    struct tm* tmstruct = localtime(&_now);
  #endif
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

void Control_LoggerFS::act() {
  needAppend = true; // Set flag to append
}

void Control_LoggerFS::dispatchPath(const String &topicPath, const String &payload ) {
  Control::dispatchPath(topicPath, payload); // Call base class dispatchPath
  if (needAppend) { // If we need to append
    needAppend = false; // Reset flag
    append(topicPath, payload);  // Normally we would subclass act() BUT need topicPath & payload
  }
}

#endif //CONTROL_LOGGER_WANT
