/*
 *  File System handling - will be used with either SD or SPIFFS
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

#ifdef SYSTEM_FS_WANT

#if defined(SYSTEM_SD_DEBUG) || defined(SYSTEM_SPIFFS_DEBUG)
  #define SYSTEM_FS_DEBUG
#endif

#include <FS.h>    // ~/Documents/Arduino/hardware/esp8266com/esp8266/cores/esp8266/FS.h
#ifdef SYSTEM_SD_WANT
  #include <SPI.h>  // SD shield for D1 mini uses SPI. https://www.arduino.cc/en/Reference/SD
  #include <SD.h>   // Defines "SD" object ~/Documents/Arduino/hardware/esp8266com/esp8266/libraries/SD/src/SD.h
  #ifndef SYSTEM_SD_PIN
    #ifdef ESP8266_D1
      #define SYSTEM_SD_PIN D4
    #elif defined(LOLIN_C3_PICO)
      #define SYSTEM_SD_PIN 6
    #else
      #error No default pin for SD cards on your board
    #endif
  #endif
#endif

#ifdef SYSTEM_SPIFFS_WANT
  #ifdef ESP32
    #define ESPFS SPIFFS // SPIFFS defind in SPIFFS.h
    #include <SPIFFS.h>
  #elif ESP8266
    #define ESPFS LittleFS // LittleFS defind in LittleFS.h
    #include <LittleFS.h>
  #endif // ESP32||ESP8266
#endif // SYSTEM_SPIFFS_WANT


#include "system_fs.h"
#include "_base.h"
#include "system_time.h" // For system_time.now
#include "misc.h" // For StringF
// May change for different boards
// #define SYSTEM_SD_CHIPSELECT D8   // SPI select pin used - note SS defined as 15 - not sure if that is D8


// Constructors
System_FS::System_FS() { }

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

#ifdef SYSTEM_SPIFFS_WANT
// Copied from system_wifi.cpp which got it from ESP-WiFiSettings library
bool System_FS::spurt(const String& filename, const String& content) {
    File f = open(filename, "w"); // Virtual, knows what kind of FS
    if (!f) return false;
    auto w = f.print(content);
    f.close();
    return w == content.length();
}
String System_FS::slurp(const String& fn) {
  File f = open(fn, "r"); // Virtual, knows what kind of FS
  String r = f.readString();
  f.close();
  return r;
}
#endif //SYSTEM_SPIFFS_WANT

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
    Serial.println("Failed to open"); Serial.println(path);
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
      #ifdef XXX110
      // TODO=110 what happened these, pretty sure could compile previously
      time_t cr = entry.GetCreationTime(); //TODO unclear which capitalization is correct for this and getLastWrite
      time_t lw = entry.getLastWrite();
      struct tm* tmstruct = localtime(&cr);
      Serial.printf("\t%d-%02d-%02d %02d:%02d:%02d", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
      tmstruct = localtime(&lw);
      Serial.printf("\t%d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
      #endif
    }
    entry.close();
  }
}

#endif // SYSTEM_FS_DEBUG

System_SD::System_SD() {}
System_SPIFFS::System_SPIFFS() {}

void System_SD::setup() {
  uint8_t pin = SYSTEM_SD_PIN;
  // Library is SS=D8=15 fails;  old sketch was 4 some online says 8 but that fatals. D4=GPIO0=2 worked on Lolin Relay with no solder bridge
  Serial.print("SD initialization on CS pin "); Serial.print(pin);
  if (!SD.begin(pin)) { // SS is defined in common.h as PIN_SPI_SS which is defined a pin 15 in https://github.com/esp8266/Arduino in variants/generic/common.h
    Serial.println(" failed!");
  } else {
    Serial.println(" done.");
    #ifdef SYSTEM_SD_DEBUG
      printDirectory("/"); // For debugging
    #endif
  }
}
void System_SPIFFS::setup() {
  #ifdef SYSTEM_SPIFFS_DEBUG
    #ifdef ESP32
      Serial.print("SPIFFS ");
    #elif ESP8266 
      Serial.print("LittleFS ");
    #endif
  #endif

  #ifdef ESP32
    if (!ESPFS.begin(true)) {
  #elif ESP8266 
    if (!ESPFS.begin()) {
  #endif
      Serial.println("initialization failed!");
  } else {
    #ifdef SYSTEM_SPIFFS_DEBUG
      Serial.println("initialization done.");
    #endif
    #ifdef SYSTEM_SPIFFS_DEBUG
      printDirectory("/"); // For debugging
    #endif
  }
}

#endif //SYSTEM_FS_WANT
