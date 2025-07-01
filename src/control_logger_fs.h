#ifndef CONTROL_LOGGER_FS_H
#define CONTROL_LOGGER_FS_H

#include "_settings.h"
#include "system_base.h"
#include "control.h"
#include "control_logger.h"
#include "system_fs.h"

class Control_LoggerFS : public Control_Logger {
  public:
    System_FS* fs; // Will be pointer to System_SD or System_LittleFS
    const char* const root; 
    // 0x01 one file per date 
    // 0x02 store topic in file
    const uint8_t strategy; // TODO turn into Enum or bit field. 
    bool needAppend;
    Control_LoggerFS(const char * const name, System_FS* fs, const char* const root, const uint8_t strategy, std::vector<IN*> i);
    void setup();
    void append(const String &topicPath, const String &payload);
    void act();
    void dispatchPath(const String &topicPath, const String &payload);
};

#endif //CONTROL_LOGGER_FS_H
