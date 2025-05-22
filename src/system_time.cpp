 /*
 * System Time Manager
 * Based on code from Jonathan Semple
 *
 * Mitra Ardron: Jan 2025
 * 
 * Required: SYSTEM_TIME_ZONE e.g. "GMT0BST,M3.5.0/1,M10.5.0" (technically optional but we are going to default it to GMT if you do not define it!)
 * Optional: SYSTEM_TIME_MS time in milliseconds to report time on serial port - usually very lomg e.g. 3600000
 * Optional: SYSTEM_TIME_DEBUG 
 */

#include "_settings.h"

#ifdef SYSTEM_TIME_WANT
#ifndef ESP32 // For now it only works on ESP32
  #error system_time only works on ESP32 for now
#endif
#include "Arduino.h"
#include <time.h>
#include "esp_sntp.h" // Not available on ESP8266
#include "system_time.h"
#include "misc.h" // for StringF

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

SystemTime::SystemTime() {}
SystemTime::~SystemTime() {}

time_t _lastSyncTime;

void NTPSyncTimeCallback(struct timeval* tv) {
  #ifdef SYSTEM_TIME_DEBUG
    Serial.println("Time: synced");
  #endif
  _lastSyncTime = (*tv).tv_sec;
}

void SystemTime::init(const char* timeZone) {
  #ifdef SYSTEM_TIME_DEBUG
    Serial.println("Time: Init");
  #endif
  timezone = timeZone;
  sntp_set_time_sync_notification_cb(NTPSyncTimeCallback);

  configTime(0, 0, "pool.ntp.org");
  #ifdef SYSTEM_TIME_DEBUG
    Serial.println("Time: Sync");
  #endif
  sync();

  setenv("TZ", timezone, 1);
  tzset();
  #ifdef SYSTEM_TIME_DEBUG
    Serial.println("Time: Init done");
  #endif
}

void SystemTime::sync() {
  if (!getLocalTime(&_localTime)) {
    #ifdef SYSTEM_TIME_DEBUG
      Serial.println("Time: Not yet synced");
    #endif
  }
}

bool SystemTime::isTimeSet() {
  time(&_now);
  return (_now > JAN_01_2024); 
}

time_t SystemTime::now() {
  time(&_now);
  localtime_r(&_now, &_localTime);
  return _now;
}

String SystemTime::dateTime() {
  return StringF("%02d/%02d/%02d %02d:%02d:%02d %s", _localTime.tm_mday, _localTime.tm_mon + 1, _localTime.tm_year > 100 ? _localTime.tm_year - 100 : _localTime.tm_year, _localTime.tm_hour, _localTime.tm_min, _localTime.tm_sec, SYSTEM_TIME_ZONE_ABBREV);
}
time_t SystemTime::lastSync() { return _lastSyncTime; }

SystemTime systemTime;

namespace xTime {  //TODO-25 - put this in a class and call from base etc

  unsigned long nextLoopTime = 0;

  void setup() {
      systemTime.init(SYSTEM_TIME_ZONE);
      systemTime.sync();
  }

  void infrequently() {
    if (nextLoopTime <= millis() ) {
      if (! systemTime.isTimeSet()) {
          Serial.print("Time since boot"); Serial.println(systemTime.now());
      } else {
          systemTime.now();
          Serial.print("Local time = "); Serial.println(systemTime.dateTime().c_str());
      }
      nextLoopTime = millis() + SYSTEM_TIME_MS;
    }
  }
} // namespace xTime

#endif // SYSTEM_TIME_WANT
