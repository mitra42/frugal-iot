/* Frugal IoT - System Power - control power managemwent 
 * 
 */
#ifndef SYSTEM_POWER_H
#define SYSTEM_POWER_H

#include "_settings.h"
#include "system_base.h"

// TO-ADD-POWER

// If need an extra bit, can assume WakeOnTimer = LightSleep
#define PauseWiFi 0x01
#define PauseMQTT 0x02
#define PauseUART 0x04
#define DeepSleep 0x08
#define DelaySleep 0x10
#define LightSleep 0x20
#define WakeOnTimer 0x40
#define WakeOnWiFi 0x80

enum System_Power_Type { 
  Power_Loop = 0,     // Standard loop, no waiting
  Power_Light = LightSleep | PauseWiFi | PauseMQTT | WakeOnTimer,        // Does a Light sleep
  Power_LightWiFi = DelaySleep | PauseMQTT,     // Like Light, but wakes on WiFi, which menas it SHOULD keep WiFi alive. (poor power savings currently - possibly because of Uart=Serial)
  Power_Modem = LightSleep | WakeOnTimer | WakeOnWiFi,       // ESP32 Modem sleep mode - need to check what this means
  Power_Deep = DeepSleep         // Does a deep sleep - resulting in a restart
};

class System_Power_Mode : public System_Base {
  public:
    System_Power_Type mode; 
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
    System_Power_Mode(const System_Power_Type mode, unsigned long cycle_ms, unsigned long wake_ms);
  protected: // Move any of these needed to public above
    unsigned long timer(uint8_t i); // Return value of timer
    unsigned long nextSleepTime = 0; // Next time to sleep in millis() (NOT offseted) - set in constructor, updated in maybeSleep()
    unsigned long cycle_ms; // Time for each cycle (wake + sleep)
    unsigned long wake_ms; // Time to stay awake during each cycle
    void setup() override;
    void LightWifi_setup();
    unsigned long sleep_ms() { return cycle_ms - wake_ms; }
    unsigned long sleep_us() { return sleep_ms() * 1000ULL; }
    //virtual void configure(); // Typically called from setup() but might also be called if switch modes
    virtual void prepare();
    virtual void sleep();
    virtual void recover();
    void dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet) override;
    void captiveLines(AsyncResponseStream* response) override;
  private:
    uint8_t timer_index;
};

#endif // SYSTEM_POWER_H

// REVIEW NEW POWER ORG  DONE BELOW  DO ABOVE.  ^^^^^^^^
