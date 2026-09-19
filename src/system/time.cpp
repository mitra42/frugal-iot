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
 * Optional: SYSTEM_TIME_S time in seconds to report time on serial port - e.g. 3600
 * 
 * Optional: SYSTEM_TIME_DEBUG 
 */

#include "_settings.h"

#include "Arduino.h"
#include <time.h>
#ifdef ESP32
  #include "esp_sntp.h" // Not available on ESP8266 but only used for sntp_set_time_sync_notification_cb which is not really needed
#endif
#include <sys/time.h> // for settimeofday/gettimeofday in set()
#include "system/time.h"
#include "misc.h" // for StringF
#include "system/frugal.h"
#include "system/language.h" // for Texts (captive portal labels)

// Nothing in the library arms a sleep-safe timer for more than SYSTEM_OTA_S (an hour), so any
// timer further out than this after a clock step is measuring against a clock that has moved.
#ifndef SYSTEM_TIME_TIMER_MAX_S
  #define SYSTEM_TIME_TIMER_MAX_S 86400
#endif

#ifdef ESP32
/* SNTP steps the clock inside the IDF, bypassing System_Time::set(), and its notification
 * callback is handed the NEW time only - the old value is already gone, so the delta cannot be
 * recovered here. Clamp instead: that needs no knowledge of the step, and bounds the damage
 * however the clock moved. The forward jump on a cold boot (1970 -> now) expires every armed
 * timer once, which is harmless; this exists for the backward case, where NTP corrects a clock
 * that a phone had set too far ahead.
 *
 * ESP8266 has no equivalent callback, so there this remains as it was.
 */
static void sntp_time_synced(struct timeval *tv) {
  (void)tv;
  frugal_iot.powercontroller->timers_clampFuture(SYSTEM_TIME_TIMER_MAX_S);
  #ifdef SYSTEM_TIME_DEBUG
    Serial.println(F("Time: NTP sync - timers clamped"));
  #endif
}
#endif

// Could pass time zone in constructor, but do not yet have any applications where local time is relevant 
#ifndef SYSTEM_TIME_ZONE
  #define SYSTEM_TIME_ZONE "GMT0BST,M3.5.0/1,M10.5.0" // Get yours at https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv 
#endif
#ifndef SYSTEM_TIME_S
  #define SYSTEM_TIME_S 360
#endif
#ifndef SYSTEM_TIME_NTP_SERVER
  #define SYSTEM_TIME_NTP_SERVER "pool.ntp.org"
#endif

#define JAN_01_2024 1704070861L
// #define TEN_MINS 600

System_Time::System_Time() 
: System_Base("time","Time"),
  timer_index(frugal_iot.powercontroller->timer_next()), // Declared first, so initialised first
  timezone(SYSTEM_TIME_ZONE) // Was left uninitialised until init() ran
  {}

System_Time::~System_Time() {}

// Initialize all the time stuff - set Timezone and start asynchronous sync with NTP 
void System_Time::init(const char* timeZone) {
  #ifdef SYSTEM_TIME_DEBUG
    Serial.println(F("Time: Init"));
  #endif
  timezone = timeZone;

  #ifdef ESP32
    sntp_set_time_sync_notification_cb(sntp_time_synced);
  #endif
  configTime(0, 0, SYSTEM_TIME_NTP_SERVER);
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

/* Formats _localTime, which localtime_r() has already put in whatever zone TZ currently names -
 * so the zone label has to come from that same zone, via strftime's %Z.
 *
 * It used to be a SYSTEM_TIME_ZONE_ABBREV #define, defaulting to "GMT" and printed whatever the
 * zone actually was. That was true while the zone could only be set at compile time, and stopped
 * being true when setTimezoneOffset() started taking one from a phone in the captive portal: a
 * node set from a browser at UTC+10 read "18:52 GMT" while 18:52 was local and GMT was 08:52.
 * A fixed label cannot describe a zone chosen at runtime, so there is no #define any more.
 *
 * %Z gives "GMT"/"BST" as the date requires for the compile-time default zone (itself better
 * than the old constant, which said GMT all summer), and the name setTimezoneOffset() built -
 * "+10" - for a browser-set one.
 */
String System_Time::dateTime() {
  // Note String is on stack so safe but not for long term use
  char buf[48];
  const size_t n = strftime(buf, sizeof(buf), "%d/%m/%y %H:%M:%S %Z", &_localTime);
  // %Z expands to nothing if this core's newlib has no zone name for TZ. %z (+1000) is computed
  // from the offset rather than looked up, so it always says something - and saying which zone
  // the time is in is the entire point of printing one.
  if (!n || buf[n - 1] == ' ') {
    strftime(buf, sizeof(buf), "%d/%m/%y %H:%M:%S UTC%z", &_localTime);
  }
  return String(buf);
}

/* Step the clock, keeping the sleep-safe timers pointing at the same real moments.
 *
 * settimeofday() moves the RTC-backed clock, which is the one that survives deep sleep - so a
 * time set once here persists across every sleep and reset short of a power cycle, which is
 * exactly what a solar node with no uplink needs.
 */
void System_Time::set(time_t epoch) {
  struct timeval before;
  gettimeofday(&before, nullptr);
  struct timeval tv;
  tv.tv_sec = epoch;
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);
  frugal_iot.powercontroller->timers_shift((int64_t)epoch - (int64_t)before.tv_sec);
  now(); // Refresh _now/_localTime so dateTime() is immediately correct
  #ifdef SYSTEM_TIME_DEBUG
    Serial.print(F("Time: set to ")); Serial.println(dateTime());
  #endif
}

/* Build a POSIX TZ string from a plain UTC offset.
 *
 * POSIX inverts the sign: "<+07>-7" means seven hours EAST of UTC. The angle-bracket form is
 * used because a numeric zone name is not a legal bare abbreviation.
 *
 * No DST rule is appended, and cannot be: a browser reports the offset it is using right now,
 * not the zone it is in, so there is nothing to derive transition dates from. That makes this
 * exactly right year-round where there is no DST - Indonesia and most of Asia - and an hour out
 * across a transition elsewhere until someone presses the button again. Encoding the zone
 * properly would need an IANA-to-POSIX table, which is real flash on a node.
 */
void System_Time::setTimezoneOffset(int16_t offset_mins) {
  const int h = abs(offset_mins) / 60;
  const int m = abs(offset_mins) % 60;
  const char nameSign = (offset_mins < 0) ? '-' : '+';
  const char posixSign = (offset_mins < 0) ? '+' : '-'; // Inverted - see above
  if (m) {
    tz_posix = StringF("<%c%02d%02d>%c%d:%02d", nameSign, h, m, posixSign, h, m);
  } else {
    tz_posix = StringF("<%c%02d>%c%d", nameSign, h, posixSign, h);
  }
  timezone = tz_posix.c_str();
  setenv("TZ", timezone, 1);
  tzset();
  now(); // Refresh _localTime through the new zone
  #ifdef SYSTEM_TIME_DEBUG
    Serial.print(F("Time: zone ")); Serial.print(timezone); Serial.print(F(" -> ")); Serial.println(dateTime());
  #endif
}

void System_Time::setup() {
  readConfigFromFS(); // Restores a previously set timezone offset - note NOT the epoch, see dispatch()
}

void System_Time::dispatch(System_Message &msg) {
  if (msg.isSet() && (msg.module() == id)) {
    if (msg.leaf() == "epoch") {
      set((time_t)strtoul(msg.payload.c_str(), nullptr, 10));
      // Echo but deliberately do NOT write to the filesystem. A stored epoch would be replayed
      // at the next boot by readConfigFromFS() and would set the clock to whenever it was last
      // written - worse than the RTC value it would be overwriting, which is still running.
      msg.maybeEcho();
    } else if (msg.leaf() == "offset") {
      setTimezoneOffset((int16_t)msg.payload.toInt());
      msg.maybeWriteToFSandEcho(); // Persisted: unlike the epoch, the zone is still true next boot
    } else {
      System_Base::dispatch(msg);
    }
  }
}

/* One line in the captive portal: what the device thinks the time is, and a button to correct it
 * from whatever is looking at the page.
 *
 * The AP is always up (System_Captive::setup calls softAP unconditionally), so this works on a
 * site with no internet and no NTP - someone walks up with a phone and the node gets a real
 * clock, which is what makes time-of-day scheduling usable there.
 *
 * s() is the portal's own helper - it POSTs one field, which arrives as set/time/<leaf>. The
 * offset is sent first so the zone is in place before the epoch lands. getTimezoneOffset() is
 * minutes WEST of UTC, hence the negation.
 */
void System_Time::captiveLines(AsyncResponseStream* response) {
  now();
  response->print(String(F("<p><label>")) + T->DeviceTime + ": " + (isTimeSet() ? dateTime() : String(T->TimeNotSet)) + "</label><br>");
  response->print(String(F("<input type=button value=\"")) + T->SetTimeFromBrowser
    + F("\" onclick=\"s('time/offset',-new Date().getTimezoneOffset());s('time/epoch',Math.floor(Date.now()/1000))\"></p>"));
}

void System_Time::setup_after_wifi() {
    // init(timezone), not init(SYSTEM_TIME_ZONE): setup() runs earlier and may already have
    // restored a browser-set zone from flash, which the compile-time default would overwrite.
    // timezone is SYSTEM_TIME_ZONE from the constructor when nothing was stored.
    init(timezone);
  // Nothing to read from disk so not calling readConfigFromFS 
  sync();
}

void System_Time::infrequently() {
  if (frugal_iot.powercontroller->timer_expired(timer_index)) {
    if (! isTimeSet()) {
        Serial.print(F("Time since boot")); Serial.println(now());
    } else {
        now();
        Serial.print(F("Local time = ")); Serial.println(dateTime().c_str());
    }
    // Was configTime(0, 0, "foo","bar","bax") - literal placeholders, which replaced the working
    // server configured in init() with three names that cannot resolve, so NTP stopped resyncing
    // after its first success. SNTP keeps polling on its own once configured, so there is nothing
    // to redo here; this block just reports.
    frugal_iot.powercontroller->timer_set(timer_index, SYSTEM_TIME_S);
  }
}
