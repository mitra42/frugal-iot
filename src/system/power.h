// Deep Sleep issues: this IS the deep sleep machinery. millis() resets, so use sleepSafeSecs(); RTC_DATA_ATTR wake_count is how a deep-sleep wake is told apart from a power-on, and is what makes setup() call recover().
/* Frugal IoT - System Power - control power managemwent 
 * 
 */
#ifndef SYSTEM_POWER_H
#define SYSTEM_POWER_H

#include "_settings.h"
#include "system/base.h"
#include "system/io.h"

/* The pins that switch power to the WHOLE node's peripherals - the outermost of the three levels.
 *
 * For the board that has one pin gating everything hanging off it, rather than a pin per sensor
 * or per bus. Turned on in pre_setup(), before anything reads anything, and off in prepare() after
 * every bus and every sensor has been powered down; back on at the head of recover(), before them.
 *
 * The LilyGo HiGrow's POWER_CTRL is exactly this, and is wired to it below - it used to be three
 * hard-coded #ifdef LILYGOHIGROW blocks in power.cpp with a TODO-115 asking for this.
 *
 * The two levels inside it are the buses (SYSTEM_I2C_POWER3v3_PIN and friends, system/interface.h)
 * and each device's own powerPins().
 */
#ifndef SYSTEM_POWER3v3_PIN
  #if defined(LILYGOHIGROW) && defined(POWER_CTRL)
    #define SYSTEM_POWER3v3_PIN POWER_CTRL
  #else
    #define SYSTEM_POWER3v3_PIN PIN_NONE
  #endif
#endif
#ifndef SYSTEM_POWER0_PIN
  #define SYSTEM_POWER0_PIN PIN_NONE
#endif

/* How long to let a rail settle after switching it on, before talking to anything on it.
 *
 * In the header rather than power.cpp because System_Frugal::setup() waits the same amount after
 * bringing the buses up at boot.
 */
#ifndef SYSTEM_POWER_ON_DELAY
  #define SYSTEM_POWER_ON_DELAY 100 // Sufficient for most sensors or actuator power to stabilize
  // Intention is to extend this if needed e.g. based on a certain sensor leave it longer
#endif

// TO-ADD-POWER

// If need an extra bit, can assume WakeOnTimerBit = LightSleepBit
#define PauseWiFiBit 0x01
#define PauseMQTTBit 0x02
#define PauseUARTBit 0x04 // Not currently used
#define DeepSleepBit 0x08
#define DelaySleepBit 0x10
#define LightSleepBit 0x20
#define WakeOnTimerBit 0x40
#define WakeOnWiFiBit 0x80

enum System_Power_Type { 
  Power_Loop = 0,     // Standard loop, no waiting
  Power_Light = LightSleepBit | PauseWiFiBit | PauseMQTTBit | WakeOnTimerBit,        // Does a Light sleep
  Power_Deep = DeepSleepBit | WakeOnTimerBit,         // Does a deep sleep - resulting in a restart
  // The following modes are experimental - supported in code but not working well for any case I am aware of
  Power_LightWiFi = DelaySleepBit | PauseMQTTBit,     // Like Light, but wakes on WiFi, which menas it SHOULD keep WiFi alive. (poor power savings currently - possibly because of Uart=Serial)
  Power_Modem = LightSleepBit | WakeOnTimerBit | WakeOnWiFiBit,       // ESP32 Modem sleep mode - need to check what this means
  Power_Panic = DeepSleepBit
};

class System_Power : public System_Base {
  public:
    System_Power_Type mode; 
    uint8_t timer_next(); // Return an index to a timer that can be used
    void timer_set(uint8_t i, uint32_t t_secs);
    /* Arm a timer for an ABSOLUTE time, rather than "t_secs from now".
     *
     * sleepSafeSecs() returns gettimeofday()'s tv_sec - the epoch itself - so the argument is
     * directly comparable with System_Time::now(): "fire at 03:00 tomorrow" is just
     * timer_set_to(i, the_epoch_of_that_moment).
     *
     * Note how this differs from timer_set() when the clock is stepped. timers_shift() preserves
     * a timer's INTERVAL, which is what timer_set() callers want (OTA, discovery, watchdog) but is
     * wrong for a wall-clock appointment - an NTP correction would slide it. So a caller using
     * this should re-arm from its own freshly computed absolute time rather than trust the value
     * it last wrote; Control_Irrigation::periodically() in examples/ospit does that every cycle
     * while idle, and is therefore self-correcting after a clock step.
     *
     * TODO This belongs in the library proper rather than arriving alongside examples/ospit - it
     * is one line and generally useful. On ospit-p2 for now, as agreed.
     */
    void timer_set_to(uint8_t i, uint32_t t_secs_absolute);
    bool timer_expired(uint8_t i);
    /* Keep armed timers sane when the wall clock is stepped.
     *
     * timer_set() stores sleepSafeSecs() + secs as an ABSOLUTE value, and sleepSafeSecs() is
     * gettimeofday() - the very clock that settimeofday() and NTP move. So stepping the clock
     * moves every armed timer relative to "now": forward, and they all expire at once; backward,
     * and they can become unreachable for years, silently stopping OTA, discovery and the
     * watchdog's periodic work on a device nobody can reach.
     *
     * Use timers_shift() where the size of the step is known (System_Time::set()), and
     * timers_clampFuture() where it is not - SNTP steps the clock inside the IDF and only tells
     * us the new time, never the old one.
     */
    void timers_shift(int64_t delta_secs);
    void timers_clampFuture(uint32_t max_secs);
    bool maybeSleep();
    void pre_setup();
    #ifdef ESP32
      // Use the RTC to track time, so safe over deep and light sleeps
      uint32_t sleepSafeSecs();
      unsigned long sleepSafeMillis();
    #else
      // ESP8266 doesnt have deep sleep so this wont reset anyway - not sure if any viable sleep on ESP8266
      uint32_t sleepSafeSecs() { return millis() / 1000; }
      unsigned long sleepSafeMillis() { return millis(); }
    #endif
    System_Power();
    void configure(System_Power_Type mode_init, unsigned long cycle_ms_init, unsigned long wake_ms_init);
    void statusLines(Print* out, bool full) override;
    /* Read the battery and, if it is below SYSTEM_POWER_LOW_MV, deep sleep HARD and FAST for
     * SYSTEM_POWER_LOW_MS, so the panel gets a chance to put something back.
     *
     * Fast is the point: below a certain voltage a dev board browns out and never reboots - an
     * ESP32-C3 will grey out and stay that way - so this sleeps without the orderly preparation
     * maybeSleep() does. See the comment in checkLevel() before adding anything to that path.
     *
     * Called from pre_setup() on every boot - which in a sleeping power mode means every wake, so
     * such a node checks continually. A node in Power_Loop boots once and then never sleeps, so
     * without periodically() below it would check exactly once, at power-on, and then run its
     * battery flat without noticing. That is the case a solar charge controller is in.
     */
    void checkLevel();
    void periodically() override;
  protected: // Move any of these needed to public above
  private:
    uint32_t timer(uint8_t i); // Return value of timer (seconds)
    unsigned long nextSleepTime = 0; // Next time to sleep in millis() (NOT offseted) - set in constructor, updated in maybeSleep()
    unsigned long cycle_ms; // Time for each cycle (wake + sleep)
    unsigned long wake_ms; // Time to stay awake during each cycle
    void setup() override;
    void LightWifi_setup();
    unsigned long sleep_ms() { return cycle_ms - wake_ms; }
    uint8_t timer_index;
    //virtual void configure(); // Typically called from setup() but might also be called if switch modes
    virtual void prepare();
    virtual void sleep(System_Power_Type forceMode = Power_Loop, unsigned long sleep_millisecs = 0);
    virtual void recover();
    // Override virtuals - so can be private 
    void dispatch(System_Message &msg) override;
    void captiveLines(AsyncResponseStream* response) override;
};

#endif // SYSTEM_POWER_H

// REVIEW NEW POWER ORG  DONE BELOW  DO ABOVE.  ^^^^^^^^
