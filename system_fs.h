#ifndef SYSTEM_FS_H
#define SYSTEM_FS_H

#include "_settings.h"

#if defined(SYSTEM_SD_DEBUG) || defined(SYSTEM_SPIFFS_DEBUG)
  #define SYSTEM_FS_DEBUG
#endif

#ifdef SYSTEM_FS_WANT
#include <FS.h>    // ~/Documents/Arduino/hardware/esp8266com/esp8266/cores/esp8266/FS.h
#ifdef SYSTEM_SD_WANT
  #include <SPI.h>  // SD shield for D1 mini uses SPI. https://www.arduino.cc/en/Reference/SD
  #include <SD.h>   // Defines "SD" object ~/Documents/Arduino/hardware/esp8266com/esp8266/libraries/SD/src/SD.h
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


class System_FS {
  public:
    System_FS() ;
    virtual void setup();
    virtual fs::File open(const char *filename, const char *mode = "r");
    virtual fs::File open(const String &filename, const char *mode = "r");
    bool spurt(const String& fn, const String& content);  // TODO-110 port to SD
    String slurp(const String& fn);  // TODO-110 port to SD
    #ifdef SYSTEM_FS_DEBUG
      String formatBytes(size_t bytes);
      void printDirectory(const char* path, int numTabs=0);
      void printDirectory(File dir, int numTabs=0);
    #endif
};

// TODO-11- maybe just merge SPIFFS (&LittleFS) & SD as interface looks same
class System_SPIFFS : public System_FS {
  public:
    System_SPIFFS();
    void setup();
    fs::File open(const char *filename, const char *mode);
    fs::File open(const String &filename, const char *mode);
};
class System_SD : public System_FS {
  public:
    System_SD();
    void setup();
    fs::File open(const char *filename, const char *mode);
    fs::File open(const String &filename, const char *mode);
};

#endif //SYSTEM_FS_WANT
#endif //SYSTEM_FS_H
