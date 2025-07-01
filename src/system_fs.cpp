/*
 *  Frugal IoT - File System handling - will be used with either SD or SPIFFS
 * 
 * There are two subclasses - System_SD and System_SPIFFS each of which expose the same API 
 * 
 * Configuration:
 * Required:
 * Optional: 

 * The WeMos Micro SD Shield uses:D5, D6, D7, D8, 3V3 and G
 * The shield uses SPI bus pins: D5 = CLK, D6 = MISO, D7 = MOSI, D8 = CS

 * Reference: https://github.com/esp8266/Arduino/blob/master/libraries/SD/examples/listfiles/listfiles.ino

 */

#include "_settings.h"
#include <Arduino.h>

#if defined(SYSTEM_SD_DEBUG) || defined(SYSTEM_SPIFFS_DEBUG)
  #define SYSTEM_FS_DEBUG
#endif

#include <FS.h>    // ~/Documents/Arduino/hardware/esp8266com/esp8266/cores/esp8266/FS.h
#include <SPI.h>  // SD shield for D1 mini uses SPI. https://www.arduino.cc/en/Reference/SD
#include <SD.h>   // Defines "SD" object ~/Documents/Arduino/hardware/esp8266com/esp8266/libraries/SD/src/SD.h

#include "system_fs.h"
#include "system_base.h"
#include "misc.h" // For StringF
// May change for different boards
// #define SYSTEM_SD_CHIPSELECT D8   // SPI select pin used - note SS defined as 15 - not sure if that is D8


// Constructors
System_FS::System_FS(const char* const id, const char* const name) 
: System_Base(id, name) { } 

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type" 
fs::File System_FS::open(const char *filename, const char *mode) { 
  Serial.print(F("should be subclassed"));
}
fs::File System_FS::open(const String &filename, const char *mode ) { 
  Serial.print(F("should be subclassed")); 
}
boolean System_FS::exists(const char *filename) {
  Serial.print(F("should be subclassed")); return false;
}
boolean System_FS::exists(const String &filename) {
  Serial.print(F("should be subclassed")); return false;
}
#pragma GCC diagnostic pop

// Basic file ops // 
// This only includes the ones we need - 
// TODO-110 feel free to add others from e.g. https://github.com/esp8266/Arduino/blob/master/libraries/SD/src/SD.h
fs::File System_SD::open(const char *filename, const char *mode) { 
  return SD.open(filename, mode);
}
fs::File System_SD::open(const String &filename, const char *mode ) { 
  return SD.open(filename, mode);
}
boolean System_SD::exists(const char *filename) {
  return SD.exists(filename);
}
boolean System_SD::exists(const String &filename) {
  return SD.exists(filename);
}
fs::File System_SPIFFS::open(const char *filename, const char *mode) { 
  return ESPFS.open(filename, mode);
}
fs::File System_SPIFFS::open(const String &filename, const char *mode) { 
  return ESPFS.open(filename, mode);
}
boolean System_SPIFFS::exists(const char *filename) {
  return ESPFS.exists(filename);
}
boolean System_SPIFFS::exists(const String &filename) {
  return ESPFS.exists(filename);
}

// Copied from system_wifi.cpp which got it from ESP-WiFiSettings library
bool System_FS::spurt(const String& filename, const String& content) {
    File f = open(filename, "w"); // Virtual, knows what kind of FS
    if (!f) {
      Serial.print(F("Failed to open for writing ")); Serial.println(filename);
      return false;
    }
    auto w = f.print(content);
    f.close();
    if (w != content.length()) {
      Serial.print(F("Failed to write to ")); Serial.println(filename);
    }
    return w == content.length();
}
String System_FS::slurp(const String& fn) {
  File f = open(fn, "r"); // Virtual, knows what kind of FS
  String r = f.readString();
  f.close();
  return r;
}

#ifdef SYSTEM_FS_DEBUG
  String System_FS::formatBytes(size_t bytes) {
    if (bytes < 1024){
      return String(bytes);
    } else if(bytes < (1024 * 1024)){
      return String(bytes/1024.0)+"K";
    } else if(bytes < (1024 * 1024 * 1024)){
      return String(bytes/1024.0/1024.0)+"M";
    } else {
      return String(bytes/1024.0/1024.0/1024.0)+"G";
    }
  }
#endif //SYSTEM_FS_DEBUG


#ifdef SYSTEM_FS_DEBUG
void System_FS::printDirectory(const char* path, int numTabs) {  // e.g. "/" 
  File dir = open(path); // TODO call via System_FS virtual 
  if (!dir) {
    Serial.println(F("Failed to open")); Serial.println(path);
  }
  printDirectory(dir, numTabs);
}

void System_FS::printDirectory(File dir, int numTabs) {  // e.g. "/" 
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) { Serial.print('\t'); }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1); // TODO want the path not the name 
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.print(formatBytes(entry.size()));
      struct tm* tmstruct;
      #ifdef ESP8266
        // For some strange reason this is missing on ESP32 FS.h
        time_t cr = entry.getCreationTime(); //TODO unclear which capitalization is correct for this and getLastWrite
        tmstruct = localtime(&cr);
        Serial.printf("\t%d-%02d-%02d %02d:%02d:%02d", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
      #endif
      time_t lw = entry.getLastWrite();
      tmstruct = localtime(&lw);
      Serial.printf("\t%d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    }
    entry.close();
  }
}

#endif // SYSTEM_FS_DEBUG

System_SD::System_SD(uint8_t pin) 
: System_FS("sd", "SD"),
  pin(pin)
  {}

System_SPIFFS::System_SPIFFS() : System_FS("spiffs", "SPIFFS") {}

void System_SD::setup() {
  // Library is SS=D8=15 fails;  old sketch was 4 some online says 8 but that fatals. D4=GPIO0=2 worked on Lolin Relay with no solder bridge
  Serial.println(F("SD initialization on CS pin ")); Serial.print(pin);
  #ifdef SYSTEM_SD_SCK // esp on ARDUINO_LOLIN_C3_PICO default pins are wrong - not those used on the shield 
    SPI.begin(SYSTEM_SD_SCK, SYSTEM_SD_MISO, SYSTEM_SD_MOSI, pin); // SCK, MISO, MOSI, pin
  #endif 
  if (!SD.begin(pin)) { 
    Serial.println(F(" failed!"));
  } else {
    Serial.println(F(" done."));
    #ifdef SYSTEM_SD_DEBUG
      printDirectory("/"); // For debugging
    #endif
  }
}
void System_SPIFFS::pre_setup() {
  #ifdef SYSTEM_SPIFFS_DEBUG
    Serial.print(F("LittleFS "));
  #endif
  if (!ESPFS.begin()) // Note it was begin(true) on SPIFFS
  {
    Serial.println(F("initialization failed!"));
  } else {
    #ifdef SYSTEM_SPIFFS_DEBUG
      Serial.println(F("initialization done."));
    #endif
    #ifdef SYSTEM_SPIFFS_DEBUG
      printDirectory("/"); // For debugging
    #endif
  }
}
