/* Frugal IoT - System Power
 * Manage power and main loop 
 * 
 * This module manages power handling, it handles decisions about when and how to sleep, and what to do when returning. 
 * 
 * There is a bitmap that defines the details, and a set of constants that group them, not all options are tested on all 
 * devices, so if in doubt stick with:
 * - Power_Loop. - never sleep;
 * - Power_Light - shorter sleeps, memory retained
 * - Power_Deep  - sleep longer periods with - memory is cleared on recovery
 * 
 * An important distinction is millis() vs sleepSafeSecs(), the latter, apart from being in seconds, keeps track over Light and Deep Sleep periods
 * 
 * This module also handles low battery. On wakeup on a device with SENSOR_BATTERY_PIN a call to configure_battery will 
 * check for a dangerously low battery, and if seen will go into a deep sleep for a configurable period before checking again
 * This is to avoid the "brown-out" condition (bug) on some chips (especially ESP-C3) that below some point will wedge and can only 
 * be brought back by power going to zero and back again.  
 * 
 * Normal control flow is .... 
 * System_Frugal::loop calls maybeSleep which if its the right time calls prepare sleep recover 
 * Except ... Deep sleep starts from scratch, like a reboot - and calls recover from setup
 * 
 * Before any sleep every module is asked okToSleep() (System_Base), and any one can refuse.
 *
 * Power_Lora (ExternalSleepBit) works in two layers - see maybeSleep() and "External sleep" below:
 *   Peripherals - maybeSleep() runs prepare() at the end of wake_ms and recover() at the end of the
 *     cycle, but does not sleep: the node is "dormant" between them, with the CPU still running.
 *   CPU - LoRaMesher light-sleeps it between its own slots, asking first through
 *     externalSleepRequest(); allowed only while dormant, and only if it will wake before the cycle ends.
 * 
 * Required: None
 *
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html
 */


#include <Arduino.h> // Required to get sdkconfig.h to get CONFIG_IDF_TARGET_xxx
#ifdef ESP32 // Not available on ESP8266 - have not yet searched for equivalents
  // Next three .h might or might not be needed
  #include "esp_pm.h"
  #include "esp_wifi.h"
  #include "esp_sleep.h"
  #include <sys/time.h>      // For gettimeofday() — RTC-backed clock, survives deep sleep
  #include "driver/uart.h"   // To allow disabling UART
  // To allow printing task list - which doesnt work anyway!
  //#include "freertos/FreeRTOS.h"
  //#include "freertos/task.h"
#endif
#include "_settings.h"
#include "system/power.h"
#include "system/interface.h"
#include "misc.h" // pinsPowerUp / pinsPowerDown - System_Power is not a System_SensorActuator
#ifdef ESP32
  #include "driver/gpio.h" // gpio_deep_sleep_hold_en
#endif
#include "system/frugal.h"

// Low battery configuration ======
#ifndef SYSTEM_POWER_LOW_MV
  #define SYSTEM_POWER_LOW_MV 3400.0
#endif
#ifndef SYSTEM_POWER_BAD_READING_MV
  // Any readings below this point are likely bad readings as device wouldnt be alive at 2.5V 
  // This may need tweaking with experience
  // Examples seen: C3Pico pin3 floating = 1792 
  #define SYSTEM_POWER_BAD_READING_MV 2500.0
#endif
//#define SYSTEM_POWER_LOW_MS 10000
#ifndef SYSTEM_POWER_LOW_MS
  #define SYSTEM_POWER_LOW_MS (60 * 60 * 1000) // Presume solar - lets go for 1 hour intervals - TODO-194 try longer or shorter levels
#endif

#ifndef SYSTEM_POWER_LOCK_WAIT_MS
  // How long an external sleep request waits for loop() to finish its pass. Comes out of
  // LoRaMesher's wake guard (100ms by default), so keep it well under that.
  #define SYSTEM_POWER_LOCK_WAIT_MS 20
#endif
#ifndef SYSTEM_POWER_EXTERNAL_MARGIN_MS
  // An external sleep must end at least this long before the cycle does - the sleep starts a little
  // after it is allowed, and maybeSleep() must find the cycle over only once the CPU is awake again.
  #define SYSTEM_POWER_EXTERNAL_MARGIN_MS 100
#endif
#ifndef SYSTEM_POWER_MIN_SLEEP_MS
  // A sleep shorter than this is skipped - prepare() and recover() cost more than it saves, at
  // least SYSTEM_POWER_ON_DELAY plus a WiFi reconnect when PauseWiFiBit is set.
  #define SYSTEM_POWER_MIN_SLEEP_MS 100
#endif

/* Not using - without a better indicator it could just leave the chip in a deep-sleep state
#ifndef SYSTEM_POWER_PANIC_MV
  #define SYSTEM_POWER_PANIC_MV 3300.0 // Still droppong 
#endif
*/
// ===============
/*
// Wont work in Arduino framework 
void printTaskList() {
  char buf[1024];
  vTaskList(buf);
  Serial.println(F("Task Name\tStatus\tPrio\tStack\tNum"));
  Serial.println(buf);
}
*/

// ==== Setup a safe place in RTC memory to store timers we want to span sleep periods. 

#define TIMER_LENGTH 8 // Can make this longer (and add to initialization) if ever need >4 timers.
#ifdef ESP32
  RTC_DATA_ATTR unsigned long wake_count = 0L; // If 1 per minute, uint_16 would just be 45 days
  // Timers stored in seconds; uint32_t rolls over at 136 years vs 49 days for millis-based unsigned long.
  RTC_DATA_ATTR uint32_t timers[TIMER_LENGTH] = {0,0,0,0,0,0,0,0}; // Array used by entities - currently using max four (OTA, Discovery, time, Watchdog)
#else
  uint32_t timers[TIMER_LENGTH] = {0,0,0,0}; // Array used by entities
#endif
//TODO-23 think about roll-over of timers and routines that test against a number maybe not rolled over

// ======

#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
  // Note how this is placed in RTC code space so it can be executed prior to the restart - it has to be short, but not sure how short
  // See https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/deep-sleep-stub.html#implement-wake-stub
  void RTC_IRAM_ATTR esp_wake_deep_sleep(void) {
      esp_default_wake_deep_sleep();
      wake_count++; // This being non-zero is used as an indication recovering from a deep sleep 
  }
#endif

// Very basic power constructor, power_configure does most of the work
System_Power::System_Power()
: System_Base("power", "Power Controller"),
  mode(Power_Loop),
  cycleStart(0L), // not sleepSafeSecs() as by definition dont sleep before this
  cycle_ms(10000),
  wake_ms(10000),
  timer_index(0)
{ }

// wake, cycle and mode are exactly what dispatch() below accepts and stores
void System_Power::statusLines(Print* out, bool full) {
  System_Base::statusLines(out, full);
  statusLine(out, "wake", String(wake_ms));
  statusLine(out, "cycle", String(cycle_ms));
  statusLine(out, "mode", String((int)mode));
}

// The power module can be configured - from the SPIFFS, Captive or MQTT 
// beware that changing power mode while running may not always do what is expected and a restart may be recommended. 
void System_Power::dispatch(System_Message &msg) {
  //TODO-23 allow changing type of power mgmt - e.g. from Deep to Loop
  if (msg.isSet() && (msg.module() == id)) {
    bool dispatched = false;
    float v = msg.payload.toFloat(); // Maybe NaN if string malformed
    if (msg.leaf() == "wake") {
      wake_ms = v;
      dispatched = true;
    } else if (msg.leaf() == "cycle") {
      cycle_ms = v;
      dispatched = true;
    } else if (msg.leaf() == "mode") {
      // TODO-23x this will almost certainly fail, switching modes isnt this easy!
      setMode((System_Power_Type)msg.payload.toInt());
      dispatched = true;
    } else {
      System_Base::dispatch(msg);
      // Drops through, dispatched=false
    }
    if (dispatched) {
      msg.maybeWriteToFSandEcho();
    }
  }
}

// Allow configuring via Captive - should probably do a device reset if change the mode (which is not currently supported here)
// TODO allow mode change from captive
void System_Power::captiveLines(AsyncResponseStream* response) {
  //TODO-23x add mode switch
  frugal_iot.captive->addNumber(response, id, "cycle", String(cycle_ms), T->CycleTime, 0, (60*60*1000));
  frugal_iot.captive->addNumber(response, id, "wake", String(wake_ms), T->WakeTime, 20000, cycle_ms);
}

// ================== manage a set of sleep-safe times - see control_blinken for example of usage =====================
// Return an index of an unused timer, note if timer_set not called it will initialize to 0L and trigger on next timer_expired
uint8_t System_Power::timer_next() {
  if (timer_index >= TIMER_LENGTH) { 
    Serial.println(F("ERROR - using too many timers"));
  }
  return timer_index++;
}
uint32_t System_Power::timer(uint8_t i) {
  return timers[i];
}
void System_Power::timer_set(const uint8_t i, const uint32_t t_secs) {
    timers[i] = sleepSafeSecs() + t_secs;
}
// Absolute form - see power.h. sleepSafeSecs() IS the epoch, so there is nothing to convert.
void System_Power::timer_set_to(const uint8_t i, const uint32_t t_secs_absolute) {
    timers[i] = t_secs_absolute;
}
bool System_Power::timer_expired(const uint8_t i) {
  return (timer(i) <= sleepSafeSecs());
}
// See power.h. A timer of 0 is left alone - that is the "never armed, fires on the first check"
// default, and shifting it would arm it.
void System_Power::timers_shift(const int64_t delta_secs) {
  for (uint8_t i = 0; i < TIMER_LENGTH; i++) {
    if (timers[i]) {
      const int64_t t = (int64_t)timers[i] + delta_secs;
      timers[i] = (t < 0) ? 0 : (uint32_t)t;
    }
  }
}
// Bound the damage from a clock step of unknown size. Nothing in the library arms a timer for
// more than SYSTEM_OTA_S (an hour), so anything further out than max_secs is not a real interval
// - it is an interval measured against a clock that has since moved. Re-arm it to fire now:
// firing one cycle early is recoverable, waiting years is not.
void System_Power::timers_clampFuture(const uint32_t max_secs) {
  const uint32_t now = sleepSafeSecs();
  for (uint8_t i = 0; i < TIMER_LENGTH; i++) {
    if (timers[i] > (now + max_secs)) {
      timers[i] = now;
    }
  }
}


// ================== setup =========== called from main.cpp::setup ========
// Board level power - a board with one pin gating its peripherals sets SYSTEM_POWER3v3_PIN (see
// power.h); the LilyGo HiGrow's POWER_CTRL is wired to it there and no longer #ifdef-ed here.

// Check level on the battery if possible, take actions to handle low battery to avoid brown out (see notes at top of file)
/* Re-check the battery once per wake cycle - see the note on checkLevel() in power.h.
 *
 */
void System_Power::periodically() {
  checkLevel();
}

void System_Power::checkLevel() {
  // Note Serial is not enabled at this point
  // TODO-194 handle case where no battery measurement possible
  if (frugal_iot.battery) {
    const float vv = frugal_iot.battery->readValidateConvert(); // Millivolts
    #ifdef SYSTEM_POWER_DEBUG
      Serial.print("Checking power at startup"); Serial.print(vv);
    #endif
    #ifdef SYSTEM_POWER_PANIC_MV // Not currently using
      if ( (vv > 1.0) && (vv < SYSTEM_POWER_PANIC_MV)) { // If its less than 1 (typically 0) then its a bad reading, ignore it
        // Low enough to risk RTC brownout and not coming back, deep sleep hard, only come back when voltage recovers
        // Not clear what happens if device just sits here at e.g. 3.1V then comes up slowly
        sleep(Power_Panic); // Power Panic is a deep sleep with no timer wakeup.
      } else 
    #endif // SYSTEM_POWER_PANIC_MV
    #ifdef SYSTEM_POWER_LOW_MV
      if ( (vv > SYSTEM_POWER_BAD_READING_MV) && (vv < SYSTEM_POWER_LOW_MV)) {
        Serial.print(vv); Serial.println("mv low voltage going go sleep");
        // options here could be .... send readings, but with long gaps; just deep sleep now for longer time (so e.g. check every 60 mins for power back)
        /* NO prepare() here, deliberately - do not "fix" this.
         *
         * checkLevel() exists to get to deep sleep FAST when the battery is low, before the rail
         * collapses far enough that the board browns out and never comes back. An ESP32-C3 in
         * particular will grey out and simply not reboot. maybeSleep() is the orderly path, where
         * there is time to power sensors down and hold pins; this one is the emergency, and
         * anything done on the way is time the battery does not have.
         *
         * The cost is that the pins are released on this path, so an output goes wherever the
         * board's pulls take it - see Actuator::preserveDuringSleep. Holding them would only be a
         * few register writes, but it would also hold a valve OPEN on a flat battery, so it is
         * not obviously the safer choice and has not been done.
         */
        sleep(Power_Deep, SYSTEM_POWER_LOW_MS);
      }
    #endif
  }
}

/* Turn on board level power to peripherals - the outermost of the three power levels.
 *
 * Called from System_Frugal::pre_setup() BEFORE checkLevel() reads the battery, because on a board
 * where one pin gates everything that pin may gate the battery divider too, and a reading taken
 * with it off would be a reading of nothing.
 *
 * The buses inside it are brought up in System_Frugal::setup(), and each sensor's own pin by its
 * setup(). See SYSTEM_POWER3v3_PIN in power.h.
 */
void System_Power::pre_setup() {
  if (pinsPowerUp(SYSTEM_POWER3v3_PIN, SYSTEM_POWER0_PIN)) {
    delay(SYSTEM_POWER_ON_DELAY); // Nothing may be read until this rail is up
  }
  #ifdef ESP32
    // Whatever the mode, since the mode can be changed later. A take and give per loop() pass is
    // all it costs when nothing else ever asks for it.
    if (!loopLock) {
      loopLock = xSemaphoreCreateMutex();
    }
  #endif
}
void System_Power::setup() {
#ifdef ESP32 // Specific to ESP32s
  if (wake_count) {
    // Only time wake_count is non-zero is if recovering from a deep sleep - its false at startup and its false when recovering from other modes but setup() isn't run in those cases.
    Serial.println(F("Power: Waking from deep"));
    // For Deep, millis is ~0 - and the sleep lasted exactly the rest of the last cycle, so a new one starts now
    cycleStart = millis(); // not sleepSafeSecs() as by definition dont deep sleep before this

    recover(); 
  } else if (mode & DelaySleepBit) { // We'll use conditions to auto enter sleep (doesnt work yet)
    LightWifi_setup();
  }
#endif
  {
    readConfigFromFS(); // Reads config (mode, wake, cycle) and passes to our dispatch
    #ifdef SYSTEM_POWER_DEBUG
      Serial.printf("Setup %s: %lu of %lu\n", name.c_str(), wake_ms, cycle_ms); 
    #endif
  }
  // esp_log_level_set("*", ESP_LOG_DEBUG); // Optional aggressive debugging! 
}

#ifdef ESP32 // LightWiFi sleep mode specific to ESP32
  // TODO Note this does not work well yet 
  void System_Power::LightWifi_setup() {
    // Note if it you get an error with esp_pm_config_t its most likely because 
    // you are using an obsolete platform: line in your platform.ini defaulting to old version of Arduino core
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 80,
        .light_sleep_enable = true
    };
    esp_pm_configure(&pm_config);   // Automatic light sleep when CPU idle

    // Enable modem sleep (WiFi will automatically enter modem sleep when possible)
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM); // or WIFI_PS_MAX_MODEM for more savings
  }
#endif

/* Every path that sets mode comes through here.
 *
 * Only ESP32 has sleep implemented - prepare(), sleep() and recover() are all ESP32 code - so
 * anywhere else the node runs Power_Loop whatever was asked for, keeping cycle_ms as the period
 * between readings. Forcing it here, rather than letting a sleeping mode run with a sleep() that
 * does nothing, means prepare() and recover() are not power-cycling sensors for no saving.
 */
void System_Power::setMode(System_Power_Type m) {
  if (dormant) { // recover() under the mode that was prepared, before that mode is forgotten
    recover();
    dormant = false;
  }
  #ifndef ESP32
    if (m != Power_Loop) {
      Serial.println(F("Power: sleep modes are ESP32 only - using Power_Loop")); // Lost if Serial not yet started
    }
    m = Power_Loop;
  #endif
  mode = m;
}

void System_Power::configure(const System_Power_Type mode_init, const unsigned long cycle_ms_init, const unsigned long wake_ms_init) {
  // Default configuration (from main.cpp) note that this should be superceeded by any settings in SPIFFS. 
  setMode(mode_init);
  cycle_ms = cycle_ms_init;
  wake_ms = wake_ms_init;
  cycleStart = millis(); // not sleepSafeSecs() as by definition dont sleep before this
  /* Serial not enabled yet
  #ifdef SYSTEM_POWER_DEBUG
    Serial.printf("%s: %lu of %lu\n", name.c_str(), wake_ms, cycle_ms); 
  #endif
  */
}

// ================== prepare =========== called from maybeSleep ========= TO-ADD-POWERMODE

// prepare - just before sleeping  (loop->maybeSleep->prepare)
void System_Power::prepare() {
  if (mode) { // Not set here ! 
    #ifdef SYSTEM_POWER_DEBUG
      Serial.println(F("Power Management: preparing"));
    #endif
    /* Power down inside out: each device's own pin, then the buses they sit on, then the whole
     * node's rail. The reverse of recover() below, and the reason the bus pass is separate from
     * the sensor pass at all - see system/interface.h.
     */
    frugal_iot.sensors->prepare();
    /* And tell the actuators, which until now were never in the sleep lifecycle at all - which is
     * why deep sleep releasing their pins had gone unnoticed. See actuator/digital.h.
     */
    frugal_iot.actuators->prepare();
    System_Interface::powerDownAll(); // Every I2C, 1-Wire and RS485 bus - after the devices on them
    pinsPowerDown(SYSTEM_POWER3v3_PIN, SYSTEM_POWER0_PIN); // And the whole-node rail, outside those
    #ifdef ESP32 // ESP8266 does not define UART_NUM_0 may be different way to shut down if relevant
      if (mode & PauseUARTBit) {
        // Need to turn anything off that could keep it awake
        // So far that isn't working - on some boards and modes (sorry, cant remember which at the moment!), some background task I can't find is stopping it sleeping
        uart_driver_delete(UART_NUM_0); // Disable UART0 (Serial)
      }
    #endif
    #ifdef INTENTIONALLY_NOT_CALLED
      if (PauseMQTTBit) { 
        frugal_iot.mqtt->pause(); // New or old session
      }
    #endif
    #ifdef ESP32 // Not built yet to pause WiFi on ESP8266
      if (mode & PauseWiFiBit) { // Note thst currently Power_Deep does not bother to pause wifi
        frugal_iot.wifi->pauseWiFi();  // Disconnect WiFi gracefully - will lose it during sleep anyway
      }
    #endif
  }
  // TODO-23 will loop through sensors and actuators here are some pointers found elsewhere...
  // WIFI 
  // see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/power_management.html for how to keep WiFi alive during sleep
  // esp_wifi_stop() // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html
  // WiFi.mode(WIFI_MODE_NULL);
  // SPI.end();
}
// ================== sleep =========== called from maybeSleep ========= TO-ADD-POWERMODE

void System_Power::sleep(System_Power_Type forceMode, unsigned long sleep_millisecs) {
  // forceMode and sleep_millisecs override default sleep - used for example to deep sleep for an hour when power low
  if (!forceMode) {
    forceMode = mode; 
  }
  if (!sleep_millisecs) { 
    sleep_millisecs = sleep_ms(); 
  };
  if (forceMode) { // != Power_Loop
    #ifdef ESP32
      if (forceMode & LightSleepBit) {
        if (forceMode & WakeOnTimerBit) {
          #ifdef SYSTEM_POWER_DEBUG
            Serial.print(F("Sleeping for ")); Serial.println(sleep_millisecs);
          #endif
          esp_sleep_enable_timer_wakeup(sleep_millisecs * 1000UL); // Wake on clock
        }
        if (forceMode & WakeOnWiFiBit) { // I don't think this is working yet
          esp_sleep_enable_wifi_wakeup();
        }
      }
    #endif
    #ifdef ESP32
      // TODO I don't think this works yet 
      // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html#_CPPv421esp_light_sleep_startv
      if (forceMode & DelaySleepBit) {
        delay(sleep_millisecs); // Light sleep will be automatic
      }
    #endif
    #ifdef ESP32
      if (forceMode & DeepSleepBit) {
        /* Make the pin holds survive the sleep itself.
         *
         * gpio_hold_en() alone is dropped as the digital domain powers down; this is what carries
         * it through. Chip-wide rather than per-pin, so it belongs here rather than in the
         * actuator - and this is the only place that knows the sleep is really a deep one, since
         * checkLevel() forces one whatever `mode` says.
         */
        gpio_deep_sleep_hold_en();
        if (forceMode & WakeOnTimerBit) {
          // millis() and esp_timer_get_time() both reset to 0 after deep sleep - see sleepSafeSecs().
          esp_deep_sleep(sleep_millisecs * 1000UL);
          Serial.println(F("Power Management: failed to go into Deep Sleep - this should not happen!"));
        } else {  // Power_Panic
          // Currently this is just to avoid brownout - deep_sleep with no wakeup and is not used due to worries it will never come back from deep_sleep unless power drops to 0
          esp_deep_sleep_start();
        }
      }
    #endif
  }
}

// ================== recover =========== called from maybeSleep ========= TO-ADD-POWERMODE
// This should undo anything done in prepare - e.g. turning devices back on
// It also has to setup the nextSleepTime - which varies from mode to mode. 

void System_Power::recover() {

  if (mode) {
    if (mode & PauseUARTBit) { // TODO not sure this works yet
      frugal_iot.startSerial(); // Note turned UART off in prepare or sleep
    }
    /* Power up outside in - the mirror of prepare(). The whole-node rail first, because it feeds
     * the buses; then the buses, because they feed the devices; then each device's own pin below.
     * Nothing is talked to until the settle delay at the end.
     */
    pinsPowerUp(SYSTEM_POWER3v3_PIN, SYSTEM_POWER0_PIN);
    System_Interface::powerUpAll();
    #ifdef SYSTEM_POWER_DEBUG
      Serial.print(F("Waking for")); Serial.println(wake_ms);
      #ifdef ACTUATOR_OLED_WANT // Maybe comment out once working
        frugal_iot.oled->display.setCursor(0,40);
        frugal_iot.oled->display.print("Recovering from Sleep");  
        frugal_iot.oled->display.display();
      #endif
    #endif
    #ifdef ESP32
      // Restore comms: Power_Loop & Power_Modem do not need it; and Power_Deep does it in setup
      bool WiFiOK = true;
      if (mode & PauseWiFiBit) {
        WiFiOK = frugal_iot.wifi->recoverWiFi();
      }
      if (WiFiOK & PauseMQTTBit) { // Note MQTT not being explicitly paused
        frugal_iot.mqtt->recover(); // New or old session
      }
      if (mode & DeepSleepBit) {
        // Memory will have been wiped by the sleep, what can we assume?
        // Note if required - can store stuff in RTC memory or on disk, but careful about overusing (writes) flash
        // Note this is called from setup() after deep sleep, which will also call other sensors & actuators to repeat setup()
        // TODO-23 test and add a note here about order - what is this before/after ? 
        #ifdef SYSTEM_POWER_DEBUG
          Serial.print(F("Power Management: recovering from Deep Sleep: ")); Serial.println(wake_count);
        #endif
        // TODO-23 be smarter about doneFullAdvertise, for example could have powered up, failed to get comms, then slept. 
        frugal_iot.discovery->doneFullAdvertise = true; // Assume did this before sleep
      }
    #endif
    // Power up sensors after sleep
    frugal_iot.sensors->recover();
    frugal_iot.actuators->recover(); // Release the pin holds and re-assert - see prepare()
    delay(SYSTEM_POWER_ON_DELAY); // Allow power to sensors and actuators to stabilize
    /* Now the rails are up and settled, begin() any bus that lost power - its pins, its UART, its
     * enumeration of what is on it all went away with it. A no-op on a bus that is not switched,
     * since initialize() is idempotent and only powerDown() clears the flag it tests.
     *
     * After the delay, deliberately: a 1-Wire scan and an RS485 begin() both talk to hardware that
     * has to be awake to answer, which is the same reason Sensor_INA219::powerUp() defers its
     * register writes rather than doing them here.
     */
    System_Interface::initializeAll();
  }
}

// ================== maybeSleep =========== called from main::loop() = doesnt return if deep-sleep ===
/* Returns true when it is time to run periodically() again - NOT an indicator that sleep occurred.
 *
 * Each cycle is awake for wake_ms from cycleStart, then asleep for whatever is left of cycle_ms.
 * Before sleeping every module is asked frugal_iot.okToSleep() - see System_Base::okToSleep(). A
 * refusal is asked again on the next loop(), and the sleep that finally happens is only for the
 * time still remaining, so cycles keep to their schedule. If the cycle ends without any sleep -
 * Power_Loop, or every attempt refused - periodically() still runs on time.
 *
 * With ExternalSleepBit (Power_Lora) there is no sleep() here: prepare() runs and the node goes
 * dormant - sensors off, WiFi paused, CPU running - until the cycle ends, when recover() runs here
 * rather than straight after. Whether the CPU sleeps meanwhile is up to LoRaMesher.
 *
 * Times are compared as millis() - cycleStart, which is safe across the 49 day millis() rollover.
 */
bool System_Power::maybeSleep() {
  bool periodicDue = false;
  const unsigned long elapsed = millis() - cycleStart;
  if (dormant) {
    if (elapsed >= cycle_ms) { // End of the dormant part of a Power_Lora cycle
      recover();
      dormant = false;
      periodicDue = true;
    }
  } else if (elapsed >= cycle_ms) {
    periodicDue = true; // Cycle over without sleeping
  } else if (mode && (elapsed >= wake_ms)) {
    const unsigned long remaining = cycle_ms - elapsed;
    // ExternalSleepBit is not ours to apply - the CPU keeps running - so do not ask about it
    if ((remaining >= SYSTEM_POWER_MIN_SLEEP_MS) && frugal_iot.okToSleep(remaining, mode & ~ExternalSleepBit)) {
      prepare(); // Get sensors ready, turn off their individual power and/or buses (I2C etc) and/or global peripheral power.
      if (mode & ExternalSleepBit) {
        dormant = true; // recover() when the cycle ends - top of this function
      } else {
        sleep(mode, remaining);
        recover(); // Never gets here in deep sleep - in that case recover is called from setup
        periodicDue = true;
      }
    }
    // else refused, or too short to be worth it - ask again next loop(), the cycle end does not move
  }
  if (periodicDue) {
    cycleStart += cycle_ms; // Anchored to the schedule, not to now - no drift
    /* Resync if that is not now within the cycle: either more than a cycle behind (e.g. a long
     * blocking periodically()), or in the future because a sleep returned early, e.g. on
     * WakeOnWiFiBit.
     */
    const long late = (long)(millis() - cycleStart);
    if ((late < 0) || ((unsigned long)late >= cycle_ms)) {
      cycleStart = millis();
    }
  }
  return periodicDue;
}

/* ================== External sleep ========== LoRaMesher's OnSleep / OnWakeUp ==================
 *
 * LoRaMesher light-sleeps the CPU itself, from its own task, between the slots of its superframe:
 * OnSleep is called once per SLEEP slot, and if allowed it then calls esp_light_sleep_start(),
 * which stops BOTH cores - loop() freezes wherever it is. OnWakeUp follows the first non-SLEEP slot
 * after a run of them, and only if at least one OnSleep in that run allowed it.
 *
 * Two things make that safe:
 *  - okToSleep() below allows it only while dormant (peripherals already off - so no prepare() or
 *    recover() is needed here, which LoRaMesher wants fast) and only if it will wake before the
 *    cycle ends, so the last slot or so of each dormant period is refused and loop() is free to
 *    recover() on time.
 *  - loopLock. loop() holds it for its whole body and lets go only around its delay(10), so an
 *    allowed sleep can never land part way through anything - a sensor read, a bus transaction,
 *    maybeSleep() recovering. The request takes it, waiting up to SYSTEM_POWER_LOCK_WAIT_MS for the
 *    current pass to finish (LoRaMesher's task outranks loopTask, so it gets it as soon as it is
 *    given), and keeps it across the whole run of sleep slots, so loop() waits in lockLoop() until
 *    OnWakeUp or a refusal. A FreeRTOS mutex must be given by the task that took it - which holds,
 *    as LoRaMesher calls both callbacks from its protocol task.
 *
 * Holding the lock also makes it safe for every module's okToSleep() to read its own state from
 * this other task: loop(), the only thing that changes that state, is held off meanwhile.
 */
bool System_Power::externalSleepRequest(const unsigned long ms) {
  bool allow = false;
  #ifdef ESP32
    if (!externalHold && loopLock) { // Already held if an earlier slot in this run was allowed
      externalHold = (xSemaphoreTake(loopLock, pdMS_TO_TICKS(SYSTEM_POWER_LOCK_WAIT_MS)) == pdTRUE);
    }
    if (externalHold) {
      allow = frugal_iot.okToSleep(ms, ExternalSleepBit | LightSleepBit | WakeOnTimerBit);
      if (!allow) { // Let loop() go now - OnWakeUp may never come, and if it does externalWake() copes
        externalHold = false;
        xSemaphoreGive(loopLock);
      }
    }
    // else loop() is in a long pass, e.g. periodically() - refuse, the next slot will ask again
  #endif
  return allow;
}
void System_Power::externalWake() {
  #ifdef ESP32
    if (externalHold) {
      externalHold = false;
      xSemaphoreGive(loopLock);
    }
  #endif
}
void System_Power::lockLoop() {
  #ifdef ESP32
    if (loopLock) {
      xSemaphoreTake(loopLock, portMAX_DELAY);
    }
  #endif
}
void System_Power::unlockLoop() {
  #ifdef ESP32
    if (loopLock) {
      xSemaphoreGive(loopLock);
    }
  #endif
}
/* Power's own say in okToSleep(). ExternalSleepBit in `how` means the request is from an external
 * sleeper - see above. Refused outright unless this is a mode that expects one: on a node in
 * Power_Loop - a gateway, typically - LoRaMesher would otherwise light-sleep the CPU and drop WiFi.
 */
bool System_Power::okToSleep(const unsigned long ms, const uint16_t how) {
  bool ok = true;
  if (how & ExternalSleepBit) {
    ok = (mode & ExternalSleepBit) && dormant
      && (((millis() - cycleStart) + ms + SYSTEM_POWER_EXTERNAL_MARGIN_MS) <= cycle_ms);
  }
  return ok;
}

// Make the current cycle already over - see power.h. Unsigned arithmetic, so fine with millis() < cycle_ms.
void System_Power::wakeNow() {
  cycleStart = millis() - cycle_ms;
}

#ifdef ESP32
  // sleepSafeSecs: esp_timer_get_time() resets to 0 on every deep sleep wake (deep sleep is a
  // full chip restart, and the ESP Timer counter lives in the digital domain that loses power).
  // gettimeofday() is anchored to the RTC domain instead, which stays powered through deep sleep,
  // so it keeps advancing across sleep/wake cycles regardless of wake source or sleep duration.
  uint32_t System_Power::sleepSafeSecs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)tv.tv_sec;
  }
  // sleepSafeMillis: same RTC-backed clock as sleepSafeSecs(), not millis() (which resets to 0
  // on deep sleep and is on a different epoch than gettimeofday() anyway).
  // Note tv_sec * 1000 does not fit in 32 bits, so this wraps every 49 days just as millis() does -
  // compare two readings by subtracting them, never with < or >.
  unsigned long System_Power::sleepSafeMillis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long)tv.tv_sec * 1000UL + (unsigned long)(tv.tv_usec / 1000);
  }
#endif
