/*
 *  Frugal IoT - File System handling - will be used with either SD or LittleFS
 * 
 * There are two subclasses - System_SD and System_LittleFS each of which expose the same API 
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

#if defined(SYSTEM_SD_DEBUG) || defined(SYSTEM_LITTLEFS_DEBUG)
  #define SYSTEM_FS_DEBUG
#endif

#include <FS.h>    // ~/Documents/Arduino/hardware/esp8266com/esp8266/cores/esp8266/FS.h
#include <SPI.h>  // SD shield for D1 mini uses SPI. https://www.arduino.cc/en/Reference/SD
#include <SD.h>   // Defines "SD" object ~/Documents/Arduino/hardware/esp8266com/esp8266/libraries/SD/src/SD.h

#include <vector>
#include "system/fs.h"
#include "system/base.h"
#include "system/io.h"
#include "misc.h" // For StringF
// May change for different boards
// #define SYSTEM_SD_CHIPSELECT D8   // SPI select pin used - note SS defined as 15 - not sure if that is D8


// Constructors
System_FS::System_FS(const char* const id, const char* const name) 
: System_Base(id, name) { } 

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type" 
#pragma GCC diagnostic ignored "-Wunused-parameter"
fs::File System_FS::open(const char *filename, const char *mode) { 
  shouldBeDefined();
}
fs::File System_FS::open(const String &filename, const char *mode ) { 
  shouldBeDefined();
  return fs::File();
}
boolean System_FS::exists(const char *filename) {
  shouldBeDefined();
  return false;
}
boolean System_FS::exists(const String &filename) {
  shouldBeDefined();
  return false; // TODO make the should-be standard, use or copy the function in system_base.cpp
}
bool System_FS::remove(const String &filename) { 
  shouldBeDefined();
  return false;
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
bool System_SD::remove(const String &filename) { 
  return SD.remove(filename);
}

fs::File System_LittleFS::open(const char *filename, const char *mode) {
  #ifdef ESP32
    return ESPFS.open(filename, mode, true);
  #elif defined(ESP8266)
    return ESPFS.open(filename, mode); // TODO need to check this creates dirs if needed for e.g. temperature/max
  #else 
    #error Need to check above for which works with any other processor
  #endif
}
fs::File System_LittleFS::open(const String &filename, const char *mode) { 
  #ifdef ESP32
    return ESPFS.open(filename, mode, true);
  #elif defined(ESP8266)
    return ESPFS.open(filename, mode); // TODO need to check this creates dirs if needed for e.g. temperature/max
  #else 
    #error Need to check above for which works with any other processor
  #endif
}
bool System_LittleFS::mkdir(const String &path) { 
  return ESPFS.mkdir(path);
}
bool System_LittleFS::rmdir(const String &path) {
  return ESPFS.rmdir(path);
}

/* Flat config paths - see the comment on these in fs.h for why the layout is flat at all.
 *
 * Encoding, in this order: '%' -> "%25", '.' -> "%2E", then the leaf's own '/' -> '.'.
 * Doing '%' first matters, or the "%2E" written for a dot would itself be re-escaped.
 * Decoding reverses it: '.' -> '/' first (an escape sequence contains no literal '.'), then
 * unescape. So "a.b/c" -> "a%2Eb.c" -> "a.b/c" round-trips, and so does "100%".
 */
String System_FS::configEncode(const String& leaf) {
  String r;
  r.reserve(leaf.length() + 8);
  for (unsigned int i = 0; i < leaf.length(); i++) {
    const char c = leaf.charAt(i);
    if (c == '%') {
      r += F("%25");
    } else if (c == '.') {
      r += F("%2E");
    } else if (c == '/') {
      r += '.';
    } else {
      r += c;
    }
  }
  return r;
}
String System_FS::configDecode(const String& encoded) {
  String r;
  r.reserve(encoded.length());
  for (unsigned int i = 0; i < encoded.length(); i++) {
    const char c = encoded.charAt(i);
    if (c == '.') {
      r += '/';
    } else if ((c == '%') && ((i + 2) < encoded.length())) {
      const String hex = encoded.substring(i + 1, i + 3);
      if (hex == "25") {
        r += '%'; i += 2;
      } else if (hex == "2E") {
        r += '.'; i += 2;
      } else {
        r += c; // Not one of ours - leave it alone rather than mangling it
      }
    } else {
      r += c;
    }
  }
  return r;
}
String System_FS::configPath(const String& id, const String& leaf) {
  return String("/") + id + "." + configEncode(leaf);
}

#ifdef SYSTEM_LITTLEFS_SUPPORTDEPRECATED
// Gather everything under one old module directory, WITHOUT changing anything yet - see the
// ordering note in convertDeprecatedLayout(). Recursive because the old layout nested a level for
// parameters: /sht/temperature/max, and /sht/temperature/value for the reading itself.
static void collectDeprecated(System_LittleFS* fs, const String& dirPath, const String& leafSoFar,
                              std::vector<String>& leaves, std::vector<String>& payloads,
                              std::vector<String>& files, std::vector<String>& dirs) {
  File dir = fs->open(dirPath, "r");
  if (dir) {
    while (true) {
      File entry = dir.openNextFile();
      if (!entry) {
        break;
      }
      const String entryName = entry.name(); // basename
      const String childPath = dirPath + "/" + entryName;
      const String childLeaf = leafSoFar.length() ? (leafSoFar + "/" + entryName) : entryName;
      if (entry.isDirectory()) {
        entry.close();
        collectDeprecated(fs, childPath, childLeaf, leaves, payloads, files, dirs);
        dirs.push_back(childPath);
      } else {
        String payload = entry.readString();
        entry.close();
        payload.trim();
        leaves.push_back(childLeaf);
        payloads.push_back(payload);
        files.push_back(childPath);
      }
    }
    dir.close();
  }
}

/* Move a board from the directory-per-module layout to the flat one, once, at startup.
 *
 * Trigger is "any directory in the root", not specifically /wifi: a board may have been
 * configured without ever joining a network, and would then have old directories but no /wifi.
 *
 * ORDER MATTERS. Everything is read into RAM first, then the old files and directories are
 * DELETED, and only then are the new files written. On a board where the old layout filled the
 * filesystem - which is the whole reason for this change - there is no room for a single new file
 * until a directory has been freed, and each one is 8KB. Reading first also means a module's
 * config survives right up to the delete; a power cut between the delete and the write loses that
 * module's settings, which is the one window this cannot close and is why it only runs once.
 */
void System_LittleFS::convertDeprecatedLayout() {
  std::vector<String> moduleDirs;
  File root = open("/", "r");
  if (root) {
    while (true) {
      File entry = root.openNextFile();
      if (!entry) {
        break;
      }
      if (entry.isDirectory()) {
        moduleDirs.push_back(String(entry.name()));
      }
      entry.close();
    }
    root.close();
  }
  if (!moduleDirs.empty()) {
    Serial.print(F("LittleFS: converting ")); Serial.print(moduleDirs.size());
    Serial.println(F(" module directories to the flat config layout"));
    for (const String& m : moduleDirs) {
      std::vector<String> leaves, payloads, files, dirs;
      collectDeprecated(this, String("/") + m, "", leaves, payloads, files, dirs);
      for (const String& f: files) {
        remove(f);
      }
      for (auto it = dirs.rbegin(); it != dirs.rend(); ++it) { // Deepest first
        rmdir(*it);
      }
      rmdir(String("/") + m);
      for (size_t i = 0; i < leaves.size(); i++) {
        const String to = configPath(m, leaves[i]);
        Serial.print(F("  /")); Serial.print(m); Serial.print(F("/")); Serial.print(leaves[i]);
        Serial.print(F(" -> ")); Serial.println(to);
        spurt(to, payloads[i]);
      }
    }
  }
}
#endif // SYSTEM_LITTLEFS_SUPPORTDEPRECATED
boolean System_LittleFS::exists(const char *filename) {
  return ESPFS.exists(filename);
}
boolean System_LittleFS::exists(const String &filename) {
  return ESPFS.exists(filename);
}
bool System_LittleFS::remove(const String &filename) { 
  return ESPFS.remove(filename);
}

// Copied from system_wifi.cpp which got it from ESP-WiFiSettings library
bool System_FS::spurt(const String& filename, const String& content) {
    bool ok = false;
    File f = open(filename, "w"); // Virtual, knows what kind of FS
    if (!f) {
      Serial.print(F("Failed to open for writing ")); Serial.println(filename);
      /* Say WHY, because the obvious guess is wrong.
       *
       * A missing directory is NOT the cause: System_LittleFS::open() passes create=true, and
       * arduino-esp32's VFSImpl::open() then walks the path calling mkdir() on each level before
       * opening. mkdir() also returns true for a directory that already exists. So the path being
       * absent cannot produce this.
       *
       * What DOES produce it: a parent that exists as a FILE (mkdir returns false and open gives
       * up), a filesystem that is not mounted, no space left, or malloc failing inside mkdir()
       * because the heap is exhausted - which this firmware used to manage during discovery.
       */
      if (!mounted) {
        Serial.println(F("  the filesystem is not mounted - see the LittleFS lines at startup"));
      }
      int slash = filename.lastIndexOf('/');
      if (slash > 0) {
        String parent = filename.substring(0, slash);
        File p = open(parent, "r");
        if (!p) {
          Serial.print(F("  parent ")); Serial.print(parent);
          Serial.println(F(" absent and could not be created - filesystem unmounted, full, or out of heap"));
        } else {
          if (!p.isDirectory()) {
            Serial.print(F("  parent ")); Serial.print(parent);
            Serial.println(F(" EXISTS AS A FILE - nothing beneath it can ever be written; delete it"));
          } else {
            Serial.print(F("  parent ")); Serial.print(parent);
            Serial.println(F(" is a directory, so this is the filesystem being full or unmounted"));
          }
          p.close();
        }
      }
    } else {
      auto w = f.print(content);
      f.close();
      if (w != content.length()) {
        Serial.print(F("Failed to write to ")); Serial.println(filename);
      }
      #ifdef SYSTEM_FS_DEBUG
        Serial.print(F("Written to:")); Serial.print(filename); Serial.print(F("=")); Serial.println(content);
      #endif
      ok = (w == content.length());
    }
    return ok;
}
// fn is path like /frugal_iot/project, note the leading slash
String System_FS::slurp(const String& fn, const bool quietfail) {
  if (quietfail && !exists(fn)) {
    //Serial.print(F("XXX __FILE")); Serial.println(__LINE__);
    return "";
  }
  //Serial.print(F("XXX __FILE")); Serial.println(__LINE__);
  File f = open(fn, "r"); // Virtual, knows what kind of FS
  String r = f.readString();
  f.close();
  return r;
}

#ifdef SYSTEM_FS_DEBUG_DIR
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
#endif //SYSTEM_FS_DEBUG_DIR


#ifdef SYSTEM_FS_DEBUG_DIR
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
      Serial.println(F("/"));
      printDirectory(entry, numTabs + 1); // TODO want the path not the name 
    } else {
      // files have sizes, directories do not
      Serial.print(F("\t\t"));
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

#endif // SYSTEM_FS_DEBUG_DIR

System_SD::System_SD(uint8_t pin) 
: System_FS("sd", "SD"),
  pin(pin)
  {}

System_LittleFS::System_LittleFS() : System_FS("littlefs", "LittleFS") {}

void System_SD::setup() {
  // Library is SS=D8=15 fails;  old sketch was 4 some online says 8 but that fatals. D4=GPIO0=2 worked on Lolin Relay with no solder bridge
  Serial.println(F("SD initialization on CS pin ")); Serial.print(pin);
  #ifdef SYSTEM_SD_SCK // esp on ARDUINO_LOLIN_C3_PICO default pins are wrong - not those used on the shield 
    // THis is no longer true - with the variant file for lolin_c3_pico,  SCK, MISO, MOSI are all correct
    SPI.begin(SYSTEM_SD_SCK, SYSTEM_SD_MISO, SYSTEM_SD_MOSI, pin); // SCK, MISO, MOSI, pin
  #endif 
  if (!SD.begin(pin)) { 
    setupFailed();
  } else {
    #ifdef SYSTEM_FS_DEBUG_DIR
      printDirectory("/"); // For debugging
    #endif
  }
}
/* Bring up LittleFS, formatting a blank partition, and SAY SO EITHER WAY.
 *
 * System_Frugal::pre_setup() calls this after startSerial(), so these prints are seen - the note
 * that used to be here about running inside the constructor before Serial is no longer true.
 *
 * The failure branch used to be an empty block with the only message commented out. That is why a
 * board whose filesystem never came up was indistinguishable from a healthy one: nothing was
 * reported here, and the first sign was every later config write failing with a per-file error
 * that pointed at the file rather than at the filesystem.
 */
void System_LittleFS::pre_setup() {
  #ifdef ESP8266
    // ESP8266/FS.cpp has no parameters to begin() and so does NOT format a non-existent filesystem
    mounted = ESPFS.begin();
  #else
    mounted = ESPFS.begin(true); // Format LittleFS if its not there
  #endif
  if (!mounted) {
    // On ESP32 begin(true) has already tried formatting, so reaching here means something more
    // basic - most often no partition for it to use at all. Try once explicitly, then say plainly
    // that nothing can be saved, because every settings write from here on will fail.
    Serial.println(F("LittleFS mount failed - formatting"));
    if (ESPFS.format()) {
      #ifdef ESP8266
        mounted = ESPFS.begin();
      #else
        mounted = ESPFS.begin(false);
      #endif
    }
    if (mounted) {
      Serial.println(F("LittleFS formatted - any previous settings are gone"));
    } else {
      Serial.println(F("LittleFS UNUSABLE - no settings can be saved."));
      Serial.println(F("  Check the partition table has a 'spiffs' entry (board_build.partitions)."));
    }
  }
  if (mounted) {
    // Unconditional, and cheap: it answers "is there a filesystem on this board, and is anything
    // in it" at a glance, which is otherwise surprisingly hard to find out on a deployed node.
    #ifndef ESP8266
      size_t used = ESPFS.usedBytes();
      size_t total = ESPFS.totalBytes();
      Serial.printf("LittleFS mounted: %u of %u bytes used\n", (unsigned)used, (unsigned)total);
      /* A filesystem that mounts but has no room is worse than one that fails to mount, because
       * begin(true) only formats when the MOUNT fails - a full one is accepted as healthy and
       * then refuses every mkdir and every write. That is usually a leftover filesystem from
       * whatever firmware was on the board before, not anything this code did.
       * Not formatted automatically: that would throw away a working node's saved settings on
       * any boot where the numbers looked bad.
       */
      if (total && (used >= total)) {
        Serial.println(F("LittleFS is FULL - no settings can be saved, and directories cannot be created."));
        // Almost always directories rather than data: each one is a metadata pair, two erase
        // blocks, 8KB here however empty it is. A 128KB partition holds the root pair plus 15.
        Serial.printf("  each directory costs %u bytes, so this partition holds %u of them\n",
          (unsigned)8192, (unsigned)((total / 8192) - 1));
        Serial.println(F("  Erase it with: pio run -t uploadfs -e <env>   (this discards saved settings)"));
      }
    #else
      Serial.println(F("LittleFS mounted"));
    #endif
    #ifdef SYSTEM_LITTLEFS_SUPPORTDEPRECATED
      convertDeprecatedLayout(); // Must be before any module calls readConfigFromFS()
    #endif
    #ifdef SYSTEM_FS_DEBUG_DIR
      printDirectory("/"); // For debugging
    #endif
  }
}
