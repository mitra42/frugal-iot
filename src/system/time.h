#ifndef SYSTEM_TIME_H
#define SYSTEM_TIME_H

#include <time.h>
#include <Arduino.h> // For String
#include "system/base.h"
#include "system/io.h"

class System_Time : public System_Base {
  public:
    uint8_t timer_index = 0;
    System_Time();
    ~System_Time();
    const char* timezone;
    void init(const char* timezone);
    time_t now();
    String dateTime();
    bool isTimeSet();
    void sync();
    /* Step the clock to epoch (seconds since 1970 UTC).
     *
     * The ONLY sanctioned way to move it: it shifts the sleep-safe timers by the same delta, so
     * they keep their intended interval - see System_Power::timers_shift for why that matters.
     * Reached from the captive portal button (a phone setting a node that has no uplink) and,
     * because it rides the message bus, from MQTT as set/time/epoch - so a gateway can set the
     * clock of a LoRa node that has no NTP of its own.
     */
    void set(time_t epoch);
    /* offset_mins is minutes EAST of UTC, i.e. what a browser reports as -getTimezoneOffset().
     * Builds a POSIX TZ string with no DST rule - see the implementation for why.
     */
    void setTimezoneOffset(int16_t offset_mins);
    void setup() override;
    void setup_after_wifi();
    void dispatch(System_Message &msg) override;
    void captiveLines(AsyncResponseStream* response) override;
    void infrequently() override;
        
    private:
        time_t _now;
        struct tm _localTime;
        // Holds the storage for whatever `timezone` points at once setTimezoneOffset has run -
        // SYSTEM_TIME_ZONE is a literal, but a browser-derived zone is built at runtime.
        String tz_posix;
};

#endif //SYSTEM_TIME_H