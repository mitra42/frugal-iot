#ifndef SYSTEM_LOGGER_H
#define SYSTEM_LOGGER_H

#include "_settings.h"
#include "_base.h"
#include "control.h"
#include "system_fs.h"

class System_Logger : public Frugal_Base {
  public:
    const char * const name; // TODO maybe push this to Frugal_Base - also in Control
    System_FS* fs; // Will be pointer to System_SD or System_SPIFFS
    const char* const root; 
    const uint8_t strategy; // TODO turn into Enum or bit field. 
    // 0x01 one file per date 
    // 0x02 store topic in file
    std::vector<IN*> inputs; // Vector of inputs
    System_Logger(const char * const name, System_FS* fs, const char* const root, const uint8_t strategy, std::vector<IN*> i);
    String advertisement();
    void setup();
    void append(const String &topicPath, const String &payload);
    void dispatch(const String& topicPath, const String& payload);
    static String advertisementAll();
    static void setupAll();
    static void dispatchAll(const String &topicPath, const String &payload);
};
extern std::vector<System_Logger*> loggers;

#endif //SYSTEM_LOGGER_H
