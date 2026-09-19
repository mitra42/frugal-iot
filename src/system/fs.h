// Deep Sleep issues: none - LittleFS is on flash, so config and logs survive.
#ifndef SYSTEM_FS_H
#define SYSTEM_FS_H

#include "_settings.h"
#include "system/base.h"
#include "system/io.h"

#if defined(SYSTEM_SD_DEBUG) || defined(SYSTEM_LITTLEFS_DEBUG)
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
  #else  // On S3T3 its SS, MOSI, MISO SCK but these are const's so cant #ifdef them - just try it and see if compiles
    #define SYSTEMS_SD_SCK SCK
    #define SYSTEM_SD_MISO MISO
    #define SYSTEM_SD_MOSI MOSI
    #define SYSTEM_SD_PIN SS
  #endif
#endif

class System_FS : public System_Base {
  public:
    System_FS(const char* const id, const char* const name);
    /* Did the filesystem actually come up? Defaults true so System_SD, which has no equivalent
     * check, behaves as before. System_LittleFS::pre_setup() sets it for real.
     *
     * Worth having as a flag rather than re-deriving it: when the filesystem is down EVERY write
     * fails, and the per-file error is then noise pointing at the wrong thing.
     */
    bool mounted = true;
    bool spurt(const String& fn, const String& content);
    /* Config lives in FLAT files at the top level: /<id>.<leaf>, e.g.
     *     /sht.temperature.max     module sht,  leaf "temperature/max"
     *     /wifi.MyNetwork          module wifi, leaf "MyNetwork" (the SSID), content = password
     *
     * It used to be a directory per module. A LittleFS directory is a metadata PAIR - two erase
     * blocks, 8KB here - however empty, so the 128KB partition held the root plus only fifteen of
     * them, and a node with more modules than that could not save anything at all.
     *
     * The leaf's own '/' separators become '.', so a literal '.' in a leaf has to be escaped or
     * it would read back as a separator. That is not hypothetical: the wifi module's leaf IS the
     * SSID, and an SSID may contain '.', '/' or '%'. Escaping '/' as well means an SSID with a
     * slash now round-trips, which it never did under the old layout - it made a nested path.
     */
    String configEncode(const String& leaf);   // "temperature/max" -> "temperature.max"
    String configDecode(const String& encoded); // "temperature.max" -> "temperature/max"
    String configPath(const String& id, const String& leaf); // -> "/sht.temperature.max"
    String slurp(const String& fn, const bool quietfail=false);
    // --- these are just the underlying FS methods exposed
    virtual fs::File open(const char *filename, const char *mode = "r");
    virtual fs::File open(const String &filename, const char *mode = "r");
    virtual boolean exists(const char *filename);
    virtual boolean exists(const String &filename);
    virtual boolean remove(const String &filename);

    // Once you have a file you should be able to append(String&) and close(); independent of whether its LittleFS LittleFS or SD

    #ifdef SYSTEM_FS_DEBUG
      String formatBytes(size_t bytes);
      void printDirectory(const char* path, int numTabs=0);
      void printDirectory(File dir, int numTabs=0);
    #endif
};

class System_LittleFS : public System_FS {
  public:
    System_LittleFS();
    void pre_setup();
    fs::File open(const char *filename, const char *mode) override;
    fs::File open(const String &filename, const char *mode) override;
    boolean exists(const char *filename) override;
    boolean exists(const String &filename) override;
    boolean remove(const String &filename);
    bool mkdir(const String &path);
    bool rmdir(const String &path);
    #ifdef SYSTEM_LITTLEFS_SUPPORTDEPRECATED
      // One-shot migration from the directory-per-module layout. Delete this, its #define, and
      // the call in pre_setup() once no board in the field still has the old layout.
      void convertDeprecatedLayout();
    #endif
};
class System_SD : public System_FS {
  public:
    uint8_t pin;
    System_SD(uint8_t pin);
    void setup() override;
    fs::File open(const char *filename, const char *mode) override;
    fs::File open(const String &filename, const char *mode) override;
    boolean exists(const char *filename) override;
    boolean exists(const String &filename) override;
    boolean remove(const String &filename);
};

#endif //SYSTEM_FS_H
