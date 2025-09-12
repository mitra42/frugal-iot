 /*
 * System Time Manager
 * Based on code from Jonathan Semple
 *
 * Mitra Ardron: Jan 2025
 * 
 * // TODO-141 pass in constructor
 * Required: SYSTEM_TIME_ZONE e.g. "GMT0BST,M3.5.0/1,M10.5.0" (technically optional but we are going to default it to GMT if you do not define it!)
 * 
 * // TODO-141 pass in constructor
 * Optional: SYSTEM_TIME_MS time in milliseconds to report time on serial port - usually very lomg e.g. 3600000
 * 
 * Optional: SYSTEM_TIME_DEBUG 
 */

#include "_settings.h"

#include "Arduino.h"
#include <time.h>
#ifdef ESP32
  #include "esp_sntp.h" // Not available on ESP8266 but only used for sntp_set_time_sync_notification_cb which is not really needed
#endif
#include "system_time.h"
#include "misc.h" // for StringF
#include "system_frugal.h" // For sleepSafemillis()

// Could pass time zone in constructor, but do not yet have any applications where local time is relevant 
#ifndef SYSTEM_TIME_ZONE
  #define SYSTEM_TIME_ZONE "GMT0BST,M3.5.0/1,M10.5.0" // Get yours at https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv 
#endif
#ifndef SYSTEM_TIME_ZONE_ABBREV
  #define SYSTEM_TIME_ZONE_ABBREV "GMT" // The typical way time is described locally 
#endif

#ifndef SYSTEM_TIME_MS
  #define SYSTEM_TIME_MS 360000 
#endif

#define JAN_01_2024 1704070861L
// #define TEN_MINS 600

System_Time::System_Time() : System_Base("time","Time") {}
System_Time::~System_Time() {}

// Initialize all the time stuff - set Timezone and start asynchronous sync with NTP 
void System_Time::init(const char* timeZone) {
  #ifdef SYSTEM_TIME_DEBUG
    Serial.println(F("Time: Init"));
  #endif
  timezone = timeZone;

  configTime(0, 0, "pool.ntp.org");
  #ifdef SYSTEM_TIME_DEBUG
    Serial.println(F("Time: Sync"));
  #endif
  sync();

  setenv("TZ", timezone, 1); // Unclear how/if this is useful
  tzset();
  #ifdef SYSTEM_TIME_DEBUG
    Serial.print(F("Time: Init done: "));
    Serial.println(dateTime());
  #endif
}
// Sync the time with NTP
void System_Time::sync() {
  if (!getLocalTime(&_localTime)) {
    #ifdef SYSTEM_TIME_DEBUG
      Serial.println(F("Time: Not yet synced"));
    #endif
  }
}

//True if time has been successfully set (with NTP)
bool System_Time::isTimeSet() {
  time(&_now);
  return (_now > JAN_01_2024); 
}

//Return time in milliseconds since Epoch
time_t System_Time::now() {
  time(&_now);
  localtime_r(&_now, &_localTime);
  return _now;
}

String System_Time::dateTime() {
  // Note String is on stack so safe but not for long term use
  return StringF("%02d/%02d/%02d %02d:%02d:%02d %s", _localTime.tm_mday, _localTime.tm_mon + 1, _localTime.tm_year > 100 ? _localTime.tm_year - 100 : _localTime.tm_year, _localTime.tm_hour, _localTime.tm_min, _localTime.tm_sec, SYSTEM_TIME_ZONE_ABBREV);
}

void System_Time::setup_after_wifi() {
    init(SYSTEM_TIME_ZONE);
  // Nothing to read from disk so not calling readConfigFromFS 
  sync();
}

void System_Time::infrequently() {
  if (nextLoopTime <= frugal_iot.powercontroller->sleepSafeMillis() ) {
    if (! isTimeSet()) {
        Serial.print(F("Time since boot")); Serial.println(now());
    } else {
        now();
        Serial.print(F("Local time = ")); Serial.println(dateTime().c_str());
    }
    nextLoopTime = (frugal_iot.powercontroller->sleepSafeMillis() + SYSTEM_TIME_MS);
    // TODO-141 check actually setting time
    configTime(0, 0, "foo","bar","bax"); // TODO this cant be right, which servers are we using ?
  }
}
