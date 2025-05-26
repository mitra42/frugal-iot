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
#include "Arduino.h"
#include <time.h>
#ifdef ESP32
  #include "esp_sntp.h" // Not available on ESP8266 but only used for sntp_set_time_sync_notification_cb which is not really needed
#endif
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

// Last time synced with NTP in seconds
time_t _lastSyncTime;

// Callback when time sync swith NTP
void NTPSyncTimeCallback(struct timeval* tv) {
  #ifdef SYSTEM_TIME_DEBUG
    Serial.println("Time: synced");
  #endif
  _lastSyncTime = (*tv).tv_sec;
}

// Initialize all the time stuff - set Timezone and start asynchronous sync with NTP 
void SystemTime::init(const char* timeZone) {
  #ifdef SYSTEM_TIME_DEBUG
    Serial.println("Time: Init");
  #endif
  timezone = timeZone;
  #ifdef ESP32
    sntp_set_time_sync_notification_cb(NTPSyncTimeCallback);
  #endif

  configTime(0, 0, "pool.ntp.org");
  #ifdef SYSTEM_TIME_DEBUG
    Serial.println(F("Time: Sync"));
  #endif
  sync();

  setenv("TZ", timezone, 1);
  tzset();
  #ifdef SYSTEM_TIME_DEBUG
    Serial.print(F("Time: Init done: "));
    Serial.println(dateTime());
  #endif
}
// Sync the time with NTP
void SystemTime::sync() {
  if (!getLocalTime(&_localTime)) {
    #ifdef SYSTEM_TIME_DEBUG
      Serial.println("Time: Not yet synced");
    #endif
  }
}

//True if time has been successfully set (with NTP)
bool SystemTime::isTimeSet() {
  time(&_now);
  return (_now > JAN_01_2024); 
}

//Return time in milliseconds since Epoch
time_t SystemTime::now() {
  time(&_now);
  localtime_r(&_now, &_localTime);
  return _now;
}

String SystemTime::dateTime() {
  // Note String is on stack so safe but not for long term use
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

<<<<<<< HEAD
  void infrequently() {
=======
  //TODO this is really only for debugging - but should have a periodic sync with NTP
  void loop() {
>>>>>>> main
    if (nextLoopTime <= millis() ) {
      if (! systemTime.isTimeSet()) {
          Serial.print("Time since boot"); Serial.println(systemTime.now());
      } else {
          systemTime.now();
          Serial.print("Local time = "); Serial.println(systemTime.dateTime().c_str());
      }
      nextLoopTime = millis() + SYSTEM_TIME_MS;
      configTime(0, 0, "foo","bar","bax");
    }
  }
} // namespace xTime

#endif // SYSTEM_TIME_WANT
