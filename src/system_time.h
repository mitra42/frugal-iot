#ifndef SYSTEM_TIME_H
#define SYSTEM_TIME_H

#include <time.h>
#include <Arduino.h> // For String
#include "system_base.h"

class System_Time : public System_Base {
  public:
    unsigned long nextLoopTime = 0; // sleepSafeMillis
    System_Time();
    ~System_Time();
    const char* timezone;
    void init(const char* timezone);
    time_t now();
    String dateTime();
    bool isTimeSet();
    void sync();
    #ifdef ESP32 // Only set from NTPSyncTimeCallback which is only called on ESP32
      time_t lastSync();
    #endif
    void setup_after_wifi();
    void infrequently();
        
    private:
        time_t _now;
        struct tm _localTime;
};

#endif //SYSTEM_TIME_H