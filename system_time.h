#ifndef SYSTEM_TIME_H
#define SYSTEM_TIME_H

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

namespace xTime {
  void setup();
  void loop();
}

#endif //SYSTEM_TIME_H