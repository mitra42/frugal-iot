/* Frugal IoT - System Power - control power managemwent 
 * 
 * Required SYSTEM_POWER_MODE_xxx
 */
#ifndef SYSTEM_POWER_H
#define SYSTEM_POWER_H

#include "_settings.h"
#include "_base.h"

// SYSTEM_POWER_MS is how often to run perioically(). 
#ifndef SYSTEM_POWER_MS
  #ifdef SYSTEM_POWER_MODE_LOOP
    #define SYSTEM_POWER_MS 10000 // Run sensors every 10 seconds 
  #elif defined(SYSTEM_POWER_MODE_LIGHT)
    #define SYSTEM_POWER_MS 60000 // Perioidically read sensors, say every 60 seconds
  #elif defined(SYSTEM_POWER_MODE_DEEP)
    #define SYSTEM_POWER_MS (60*60*1000) // In low power mode, it might be infrquent e.g. every hour
  #else
    #error "Must define one of SYSTEM_POWER_MODE_LOOP _MEDIUM or _LOW"
  #endif
#endif
// SYSTEM_POWER_WAKE_MS is how long to stay awake before sleeping - say 10 seconds to allow for queued messages - maybe less 
#ifndef SYSTEM_POWER_WAKE_MS
  #if defined(SYSTEM_POWER_MODE_LOOP)
    #define SYSTEM_POWER_WAKE_MS SYSTEM_POWER_MS // In high power mode, always awake
  #elif defined(SYSTEM_POWER_MODE_LIGHT)
    #define SYSTEM_POWER_WAKE_MS 10000 // Long enough for queued messages etc say 10 seconds.
  #elif defined(SYSTEM_POWER_MODE_DEEP)
    #define SYSTEM_POWER_WAKE_MS 30000 // In low power mode, has to be long enough to connect to WiFi and MQTT, so 30 seconds
  #else
    #error "Must define one of SYSTEM_POWER_MODE_LOOP _MEDIUM or _LOW"
  #endif
#endif


class System_Power_Mode : public Frugal_Base {
  public:
    unsigned long nextSleepTime = 0; // Next time to sleep in millis() (NOT offseted) - set in constructor, updated in maybeSleep()
    unsigned long cycle_ms; // Time for each cycle (wake + sleep)
    unsigned long wake_ms; // Time to stay awake during each cycle
    System_Power_Mode(const char* name, unsigned long cycle_ms, unsigned long wake_ms);
    virtual void setup();
    unsigned long sleep_ms() { return cycle_ms - wake_ms; }
    unsigned long sleep_us() { return sleep_ms() * 1000ULL; }
    bool maybeSleep();
    //virtual void configure(); // Typically called from setup() but might also be called if switch modes
    virtual void prepare();
    virtual void sleep();
    virtual void recover();
    #ifdef ESP32
      unsigned long sleepSafeMillis();
    #else // Only needed/valid on ESP32 where have saved millis_offset in RTCs memory
      unsigned long sleepSafeMillis() { return millis(); }
    #endif
};
class System_Power_Mode_Loop : public System_Power_Mode {
  public:
    System_Power_Mode_Loop(unsigned long cycle_ms, unsigned long wake_ms);
    //void configure(); // Typically called from setup() but might also be called if switch modes
    //void setup();
    void prepare();  // Does nothing in Loop
    void sleep(); // Does nothing in Loop
    void recover(); // Does nothing in Loop
};
#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
class System_Power_Mode_Light : public System_Power_Mode {
  public:
    System_Power_Mode_Light(unsigned long cycle_ms, unsigned long wake_ms);
    //void configure(); // Typically called from setup() but might also be called if switch modes
    void prepare();
    void sleep();
    void recover();
};
#endif
#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32

class System_Power_Mode_Deep : public System_Power_Mode {
  public:
    System_Power_Mode_Deep(unsigned long cycle_ms, unsigned long wake_ms);
   // void configure(); // Typically called from setup() but might also be called if switch modes
    void setup();
    //void prepare(); // Use superclass
    void sleep();
    void recover();
};
#endif
#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32

class System_Power_Mode_LightWifi : public System_Power_Mode {
  public:
    System_Power_Mode_LightWifi(unsigned long cycle_ms, unsigned long wake_ms);
    //void configure(); // Typically called from setup() but might also be called if switch modes
    void setup();
    //void prepare();
    void sleep();
    void recover();
};
#endif
#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32

class System_Power_Mode_Modem : public System_Power_Mode {
  public:
    System_Power_Mode_Modem(unsigned long cycle_ms, unsigned long wake_ms);
    //void configure(); // Typically called from setup() but might also be called if switch modes
    //void prepare();
    void sleep();
    //void recover();
};
#endif

extern System_Power_Mode* powerController;

#endif // SYSTEM_POWER_H