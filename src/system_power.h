/* Frugal IoT - System Power - control power managemwent 
 * 
 */
#ifndef SYSTEM_POWER_H
#define SYSTEM_POWER_H

#include "_settings.h"
#include "system_base.h"

// TO-ADD-POWER
enum System_Power_Type { 
  Power_Loop,         // Standard loop, no waiting
  Power_Light,        // Does a Light sleep
  Power_LightWiFi,    // Like Light, but wakes on WiFi, which menas it SHOULD keep WiFi alive. (poor power savings currently - possibly because of Uart=Serial)
  Power_Modem,        // ESP32 Modem sleep mode - need to check what this means
  Power_Deep          // Does a deep sleep - resulting in a restart
};

class System_Power_Mode : public System_Base {
  public:
    uint8_t timer_next(); // Return an index to a timer that can be used
    void timer_set(uint8_t i, unsigned long t);
    bool timer_expired(uint8_t i); 
    bool maybeSleep();
    void pre_setup();
    #ifdef ESP32
      unsigned long sleepSafeMillis();
    #else // Only needed/valid on ESP32 where have saved millis_offset in RTCs memory
      unsigned long sleepSafeMillis() { return millis(); }
    #endif
    static System_Power_Mode* create(System_Power_Type, unsigned long cycle_ms, unsigned long wake_ms);
  protected: // Move any of these needed to public above
    unsigned long timer(uint8_t i); // Return value of timer
    unsigned long nextSleepTime = 0; // Next time to sleep in millis() (NOT offseted) - set in constructor, updated in maybeSleep()
    unsigned long cycle_ms; // Time for each cycle (wake + sleep)
    unsigned long wake_ms; // Time to stay awake during each cycle
    System_Power_Mode(const char* name, unsigned long cycle_ms, unsigned long wake_ms);
    void setup() override;
    unsigned long sleep_ms() { return cycle_ms - wake_ms; }
    unsigned long sleep_us() { return sleep_ms() * 1000ULL; }
    //virtual void configure(); // Typically called from setup() but might also be called if switch modes
    virtual void prepare();
    virtual void sleep();
    virtual void recover();
  private:
    uint8_t timer_index;
};
class System_Power_Loop : public System_Power_Mode {
  public:
    System_Power_Loop(unsigned long cycle_ms, unsigned long wake_ms);
    //void configure(); // Typically called from setup() but might also be called if switch modes
    //void setup() override;
    void prepare();  // Does nothing in Loop
    void sleep(); // Does nothing in Loop
    void recover(); // Does nothing in Loop
};
#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
class System_Power_Light : public System_Power_Mode {
  public:
    System_Power_Light(unsigned long cycle_ms, unsigned long wake_ms);
    //void configure(); // Typically called from setup() but might also be called if switch modes
    void prepare() override;
    void sleep() override;
    void recover() override;
};
#endif
#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32

class System_Power_Deep : public System_Power_Mode {
  public:
    System_Power_Deep(unsigned long cycle_ms, unsigned long wake_ms);
   // void configure(); // Typically called from setup() but might also be called if switch modes
    void setup() override;
    //void prepare() override; // Use superclass
    void sleep() override;
    void recover() override;
};
#endif
#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32

class System_Power_LightWiFi : public System_Power_Mode {
  public:
    System_Power_LightWiFi(unsigned long cycle_ms, unsigned long wake_ms);
    //void configure(); // Typically called from setup() but might also be called if switch modes
    void setup() override;
    //void prepare() override;
    void sleep() override;
    void recover() override;
};
#endif
#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32

class System_Power_Modem : public System_Power_Mode {
  public:
    System_Power_Modem(unsigned long cycle_ms, unsigned long wake_ms);
    //void configure(); // Typically called from setup() but might also be called if switch modes
    //void prepare() override;
    void sleep() override;
    //void recover() override;
};
#endif
extern System_Power_Mode* powerController;

#endif // SYSTEM_POWER_H