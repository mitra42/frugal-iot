/* Frugal IoT - System Power - control power managemwent 
 * 
 */
#ifndef SYSTEM_POWER_H
#define SYSTEM_POWER_H

#include "_settings.h"
#include "_base.h"

#if !defined(SYSTEM_POWER_MODE_HIGH) && !defined(SYSTEM_POWER_MODE_MEDIUM) && !defined(SYSTEM_POWER_MODE_LOW)
  #define SYSTEM_POWER_MODE_MEDIUM // Default to high power mode if none defined
#endif
// SYSTEM_POWER_MS is how often to run perioically(). 
#ifndef SYSTEM_POWER_MS
  #ifdef SYSTEM_POWER_MODE_HIGH
    #define SYSTEM_POWER_MS 10000 // Run sensors every 10 seconds 
  #elif defined(SYSTEM_POWER_MODE_MEDIUM)
    #define SYSTEM_POWER_MS 60000 // Perioidically read sensors, say every 60 seconds
  #elif defined(SYSTEM_POWER_MODE_LOW)
    #define SYSTEM_POWER_MS (60*60*1000) // In low power mode, it might be infrquent e.g. every hour
  #else
    #error "Must define one of SYSTEM_POWER_MODE_HIGH _MEDIUM or _LOW"
  #endif
#endif
// SYSTEM_POWER_WAKE_MS is how long to stay awake before sleeping - say 10 seconds to allow for queued messages - maybe less 
#ifndef SYSTEM_POWER_WAKE_MS
  #if defined(SYSTEM_POWER_MODE_HIGH)
    #define SYSTEM_POWER_WAKE_MS SYSTEM_POWER_MS // In high power mode, always awake
  #elif defined(SYSTEM_POWER_MODE_MEDIUM)
    #define SYSTEM_POWER_WAKE_MS 10000 // Long enough for queued messages etc say 10 seconds.
  #elif defined(SYSTEM_POWER_MODE_LOW)
    #define SYSTEM_POWER_WAKE_MS 30000 // In low power mode, has to be long enough to connect to WiFi and MQTT, so 30 seconds
  #else
    #error "Must define one of SYSTEM_POWER_MODE_HIGH _MEDIUM or _LOW"
  #endif
#endif
#define SYSTEM_POWER_SLEEP_MS (SYSTEM_POWER_MS - SYSTEM_POWER_WAKE_MS) // How long to sleep in microseconds
#define SYSTEM_POWER_SLEEP_US (SYSTEM_POWER_SLEEP_MS * 1000ULL) // How long to sleep in microseconds


class System_Power : public Frugal_Base {
  public:
    unsigned long nextSleepTime = 0; // Next time to sleep in millis() (NOT offseted) - set in constructor, updated in maybeSleep()
    System_Power();
    void setup();
    void prepareForDeepSleep();
    void recoverFromDeepSleep();
    void prepareForLightSleep();
    void recoverFromLightSleep();
    bool maybeSleep();
    unsigned long sleepSafeMillis(); 
};

extern System_Power* powerController;

#endif // SYSTEM_POWER_H