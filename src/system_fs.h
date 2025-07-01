#ifndef SYSTEM_FS_H
#define SYSTEM_FS_H

#include "_settings.h"
#include "system_base.h"

#if defined(SYSTEM_SD_DEBUG) || defined(SYSTEM_SPIFFS_DEBUG)
  #define SYSTEM_FS_DEBUG
#endif

#include <FS.h>    // ~/Documents/Arduino/hardware/esp8266com/esp8266/cores/esp8266/FS.h
#ifdef SYSTEM_SD_WANT
  #include <SPI.h>  // SD shield for D1 mini uses SPI. https://www.arduino.cc/en/Reference/SD
  #include <SD.h>   // Defines "SD" object ~/Documents/Arduino/hardware/esp8266com/esp8266/libraries/SD/src/SD.h
#endif
#define ESPFS LittleFS // LittleFS defind in LittleFS.h
#include <LittleFS.h>

// Define defuault SD Pin if known - constructor can use if not specified
#ifndef SYSTEM_SD_PIN
  #ifdef ESP8266_D1
    #define SYSTEM_SD_PIN D4 // Default pin on the shield - if override theres a solder bridge to change
  #elif defined(ARDUINO_LOLIN_C3_PICO) || defined(ARDUINO_LOLIN_S2_MINI)
    #define SYSTEM_SD_SCK 1
    #define SYSTEM_SD_MISO 0
    #define SYSTEM_SD_MOSI 4
    #define SYSTEM_SD_PIN 6 // Default pin on the shield - if override theres a solder bridge to change
  #endif
#endif

class System_FS : public System_Base {
  public:
    System_FS(const char* const id, const char* const name);
    bool spurt(const String& fn, const String& content);  // TODO-110 port to SD
    String slurp(const String& fn);  // TODO-110 port to SD

    // --- these are just the underlying FS methods exposed
    virtual fs::File open(const char *filename, const char *mode = "r");
    virtual fs::File open(const String &filename, const char *mode = "r");
    virtual boolean exists(const char *filename);
    virtual boolean exists(const String &filename);

    // Once you have a file you should be able to append(String&) and close(); independent of whether its SPIFFS LittleFS or SD

    #ifdef SYSTEM_FS_DEBUG
      String formatBytes(size_t bytes);
      void printDirectory(const char* path, int numTabs=0);
      void printDirectory(File dir, int numTabs=0);
    #endif
};

class System_SPIFFS : public System_FS {
  public:
    System_SPIFFS();
    void pre_setup();
    fs::File open(const char *filename, const char *mode);
    fs::File open(const String &filename, const char *mode);
    virtual boolean exists(const char *filename);
    virtual boolean exists(const String &filename);

};
class System_SD : public System_FS {
  public:
    uint8_t pin;
    System_SD(uint8_t pin);
    void setup();
    fs::File open(const char *filename, const char *mode);
    fs::File open(const String &filename, const char *mode);
    virtual boolean exists(const char *filename);
    virtual boolean exists(const String &filename);
};

#endif //SYSTEM_FS_H
