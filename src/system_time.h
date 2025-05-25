#ifndef SYSTEM_TIME_H
#define SYSTEM_TIME_H

#include <time.h>
#include <Arduino.h> // For String

class SystemTime {
    public:
        SystemTime();
        ~SystemTime();
        const char* timezone;
        void init(const char* timezone);
        time_t now();
        String dateTime();
        bool isTimeSet();
        void sync();
        time_t lastSync();
        
    private:
        time_t _now;
        struct tm _localTime;
};

extern SystemTime systemTime;

namespace xTime { // TODO move these into the FrugalBase etc setup and loop and make methods of SystemTime
  void setup();
  void loop();
}

#endif //SYSTEM_TIME_H