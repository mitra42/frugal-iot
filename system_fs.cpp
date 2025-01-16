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
#define SYSTEM_FS_DEBUG
#include <Arduino.h>

#ifdef SYSTEM_FS_WANT
#include <FS.h>    // ~/Documents/Arduino/hardware/esp8266com/esp8266/cores/esp8266/FS.h
#include <SPI.h>  // SD shield for D1 mini uses SPI. https://www.arduino.cc/en/Reference/SD
#include <SD.h>   // Defines "SD" object ~/Documents/Arduino/hardware/esp8266com/esp8266/libraries/SD/src/SD.h
#include "system_fs.h"

// May change for different boards
#define SYSTEM_SD_CHIPSELECT D8   // SPI select pin used

class System_FS {
  System_FS() ;
  virtual void setup();
  #ifdef SYSTEM_FS_DEBUG
    String formatBytes(size_t bytes);
  #endif
};

/*
class System_SPIFFS {
  void setup();
}
*/
class System_SD {
  System_SD();
  void setup();
};

System_FS::System_FS() { }

#ifdef SYSTEM_FS_DEBUG
  String System_FS::formatBytes(size_t bytes) {
    if (bytes < 1024){
      return String(bytes)+"B";
    } else if(bytes < (1024 * 1024)){
      return String(bytes/1024.0)+"KB";
    } else if(bytes < (1024 * 1024 * 1024)){
      return String(bytes/1024.0/1024.0)+"MB";
    } else {
      return String(bytes/1024.0/1024.0/1024.0)+"GB";
    }
  }
#endif //SYSTEM_FS_DEBUG

#ifdef SYSTEM_FS_DEBUG

void System_FS::printDir(const char* path, int numTabs=0) {  // e.g. "/" 
  File dir = SD.open(path) // TODO call via System_FS virtual 
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
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.print(entry.size(), DEC);   //TODO use formatBytes
      time_t cr = entry.getCreationTime();
      time_t lw = entry.getLastWrite();
      struct tm* tmstruct = localtime(&cr);
      Serial.printf("\tCREATION: %d-%02d-%02d %02d:%02d:%02d", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
      tmstruct = localtime(&lw);
      Serial.printf("\tLAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    }
    entry.close();
  }
}

#endif // SYSTEM_FS_DEBUG

System_SD::System_SD() {}

void System_SD::setup() {
  if (!SD.begin(SS)) {
    Serial.println("XXX initialization failed!");
    return;
  }
  Serial.println("XXX initialization done.");
}
#endif //SYSTEM_FS_WANT
