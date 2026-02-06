/* Frugal IoT - System Power
 * Manage power and main loop 
 * 
 * Required: 
 *
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html
 */


#include <Arduino.h> // Required to get sdkconfig.h to get CONFIG_IDF_TARGET_xxx
#ifdef ESP32 // Not available on ESP8266 - have not yet searched for equivalents
  // Next three .h might or might not be needed 
  #include "esp_pm.h"
  #include "esp_wifi.h"
  #include "esp_sleep.h"
  #include "driver/uart.h"   // To allow disabling UART
  // To allow printing task list - which doesnt work anyway!
  //#include "freertos/FreeRTOS.h"
  //#include "freertos/task.h"
#endif
#include "_settings.h"
#include "system_power.h"
#include "system_frugal.h"

#ifndef SYSTEM_POWER_LOW_MV
  #define SYSTEM_POWER_LOW_MV 3400.0
#endif
//#define SYSTEM_POWER_LOW_MS 10000
#ifndef SYSTEM_POWER_LOW_MS
  #define SYSTEM_POWER_LOW_MS (60 * 60 * 1000) // Presume solar - lets go for 1 hour intervals - TODO-194 try longer or shorter levels
#endif
/* Not using - without a better indicator it could just leave the chip in a deep-sleep state
#ifndef SYSTEM_POWER_PANIC_MV
  #define SYSTEM_POWER_PANIC_MV 3300.0 // Still droppong 
#endif
*/
/*
// Wont work in Arduino framework 
void printTaskList() {
  char buf[1024];
  vTaskList(buf);
  Serial.println(F("Task Name\tStatus\tPrio\tStack\tNum"));
  Serial.println(buf);
}
*/
#define TIMER_LENGTH 8 // Can make this longer (and add to initialization) if ever need >4 timers.
#ifdef ESP32
  RTC_DATA_ATTR unsigned long wake_count = 0L; // If 1 per minute, uint_16 would just be 45 days
  RTC_DATA_ATTR unsigned long millis_offset = 0L;  // Cumulative time in deep sleep with millis() being reset
  // This is an array of timers, available to modules
  RTC_DATA_ATTR unsigned long timers[TIMER_LENGTH] = {0L,0L,0L,0L,0L,0L,0L,0L}; // Array used by entities - currently using max five (OTA, Disovery. Blinken, time, Watchdog)
#else
  unsigned long timers[TIMER_LENGTH] = {0L,0L,0L,0L}; // Array used by entities
#endif
//TODO-23 think about roll-over of timers and routines that test against a number maybe not rolled over

#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
// Note how this is placed in RTC code space so it can be executed prior to the restart - it has to be short, but not sure how short
// See https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/deep-sleep-stub.html#implement-wake-stub
void RTC_IRAM_ATTR esp_wake_deep_sleep(void) {
    esp_default_wake_deep_sleep();
    wake_count++; // This being non-zero is used as an indication recovering from a deep sleep 
}
#endif

System_Power::System_Power()
: System_Base("power", "Power Controller"),
  mode(Power_Loop),
  nextSleepTime(0L), // not sleepSafeMillis() as by definition dont sleep before this
  cycle_ms(10000),
  wake_ms(10000),
  timer_index(0)
{ }
// This is for MQTT messages addressed at the mqtt module e.g. dev/org/node/set/mqtt/hostname
void System_Power::dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, const bool isSet) {
  //TODO-23 allow changing type of power mgmt - e.g. from Deep to Loop
  if (isSet && (topicSensorId == id)) {
    bool dispatched = false;
    float v = payload.toFloat(); // Maybe NaN if string malformed
    if (topicTwig == "wake") {      
      wake_ms = v;
      dispatched = true;
    } else if (topicTwig == "cycle") {
      cycle_ms = v;
      dispatched = true;
    } else if (topicTwig == "mode") {
        // TODO-23x this will almost certainly fail, switching modes isnt this easy! 
        mode = (System_Power_Type)payload.toInt();;
        dispatched = true;
    } else {
      System_Base::dispatchTwig(topicSensorId, topicTwig, payload, isSet);
      // Drops through, dispatched=false
    }
    if (dispatched) {
      writeConfigToFS(topicTwig, payload);
    } 
  }
}

void System_Power::captiveLines(AsyncResponseStream* response) {
  //TODO-23x add mode switch
  frugal_iot.captive->addNumber(response, id, "cycle", String(cycle_ms), "Cycle time (ms)", 0, (60*60*1000));
  frugal_iot.captive->addNumber(response, id, "wake", String(wake_ms), "Wake time (ms)", 20000, cycle_ms);
}

// ================== manage a set of sleep-safe times - see control_blinken for example of usage =====================
// Return an index of an unused timer, note if timer_set not called it will initialize to 0L and trigger on next timer_expired
uint8_t System_Power::timer_next() {
  if (timer_index >= TIMER_LENGTH) { 
    Serial.println(F("ERROR - using too many timers"));
  }
  return timer_index++;
}
unsigned long System_Power::timer(uint8_t i) {
  return timers[i];
}
void System_Power::timer_set(const uint8_t i, const unsigned long t) {
    timers[i] = sleepSafeMillis() + t ;
}
bool System_Power::timer_expired(const uint8_t i) {
  return (timer(i) <= sleepSafeMillis());
}


// ================== setup =========== called from main.cpp::setup ========
// This section can contain ifdef-ed parts that manage things like power at the board level such as on LILYGOHIGROW

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
        sleep(Power_Panic);
      } else 
    #endif // SYSTEM_POWER_PANIC_MV
    #ifdef SYSTEM_POWER_LOW_MV
      if ( (vv > 1.0) && (vv < SYSTEM_POWER_LOW_MV)) {
        Serial.println(" low power going sleep");
        // options here could be .... send readings, but with long gaps; just deep sleep now for longer time (so e.g. check every 60 mins for power back)
        sleep(Power_Deep, SYSTEM_POWER_LOW_MS);
      }
    #endif
  }
}

void System_Power::pre_setup() {
  #ifdef LILYGOHIGROW
    pinMode(POWER_CTRL, OUTPUT);
    digitalWrite(POWER_CTRL, HIGH); // TODO-115 this is for power control - may need other board specific stuff somewhere
  #endif
}
void System_Power::setup() {
#ifdef ESP32 // Specific to ESP32s
  if (wake_count) {
    // Only time wake_count is non-zero is if recovering from a deep sleep - its false at startup and its false when recovering from other modes but setup() isn't run in those cases.
    recover(); 
  } else if (mode & DelaySleep) { // We'll use conditions to auto enter sleep (doesnt work yet)
    LightWifi_setup();
  }
#endif
  {
    readConfigFromFS(); // Reads config (mode, wake, cycle) and passes to our dispatchTwig
    #ifdef SYSTEM_POWER_DEBUG
      Serial.printf("Setup %s: %lu of %lu\n", name.c_str(), wake_ms, cycle_ms); 
    #endif
  }
  // esp_log_level_set("*", ESP_LOG_DEBUG); // Optional aggressive debugging! 
}

#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
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
// Currently on ESP8266 just support Power_Loop
void System_Power::configure(const System_Power_Type mode_init, const unsigned long cycle_ms_init, const unsigned long wake_ms_init) {
  mode = mode_init;
  cycle_ms = cycle_ms_init;
  wake_ms = wake_ms_init;
  nextSleepTime = (millis() + wake_ms); // not sleepSafeMillis() as by definition dont sleep before this
  /* Serial not enabled yet
  #ifdef SYSTEM_POWER_DEBUG
    Serial.printf("%s: %lu of %lu\n", name.c_str(), wake_ms, cycle_ms); 
  #endif
  */
}

// ================== prepare =========== called from maybeSleep ========= TO-ADD-POWERMODE


// prepare - run from loop (or maybeSleep) just before sleeping 
void System_Power::prepare() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println(F("Power Management: preparing"));
  #endif
  if (mode) { // Not set here ! 
    // Some things wont be done if just looping
    #ifdef LILYGOHIGROW
      digitalWrite(POWER_CTRL, LOW);
    #endif
    #ifdef ESP32 // ESP8266 does not define UART_NUM_0 may be different way to shut down if relevant
      if (mode & PauseUART) {
        // Need to turn anything off that could keep it awake
        // So far that isn't working - some background task I can't find is stopping it sleeping
        uart_driver_delete(UART_NUM_0); // Disable UART0 (Serial)
      }
    #endif
    #ifdef ESP32 // Not built yet to pause WiFi on ESP8266
      if (mode & PauseWiFi) {
        frugal_iot.wifi->pause();  // Disconnect WiFi gracefully - will lose it during sleep anyway
        // TODO-23 note that mqtt and loramesher both define prepareForLightSleep but it isn't called
      }
    #endif
  }
  // TODO-23 will loop through sensors and actuators here are some pointers found elsewhere...
  // WIFI 
  // see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/power_management.html for how to keep WiFi alive during sleep
  // esp_wifi_stop() // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html
  // WiFi.mode(WIFI_MODE_NULL);
  // LoRa.sleep();
  // SPI.end();
}
// ================== sleep =========== called from maybeSleep ========= TO-ADD-POWERMODE

void System_Power::sleep(System_Power_Type forceMode, unsigned long sleep_millisecs) {
  if (!forceMode) {
    forceMode = mode; 
  }
  if (!sleep_millisecs) { 
    sleep_millisecs = sleep_ms(); 
  };
  if (forceMode) { // != Power_Loop
    #ifdef ESP32
      if (forceMode & LightSleep) {
        if (forceMode & WakeOnTimer) {
          #ifdef SYSTEM_POWER_DEBUG
            Serial.print(F("Sleeping for ")); Serial.println(sleep_ms());
          #endif
          esp_sleep_enable_timer_wakeup(sleep_millisecs * 1000UL); // Wake on clock
        }
        if (forceMode & WakeOnWiFi) {
          esp_sleep_enable_wifi_wakeup();
        }
      }
    #endif
    #ifdef ESP32
      // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html#_CPPv421esp_light_sleep_startv
      if (forceMode & DelaySleep) {
        delay(sleep_millisecs); // Light sleep will be automatic
      }
    #endif
    #ifdef ESP32
      if (forceMode & DeepSleep) {
        if (forceMode & WakeOnTimer) {
          millis_offset = millis() + millis_offset + sleep_millisecs; // Since millis() will reset to 0 - stored in RTC
          esp_deep_sleep(sleep_millisecs * 1000UL);
          Serial.println(F("Power Management: failed to go into Deep Sleep - this should not happen!"));
        } else {  // Currently this is just to avoid brownout - deep_sleep with no wakeup and is not used due to worries it will never come back from deep_sleep unless power drops to 0
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
  // For Power_Loop wake_ms = cycle_ms; for Deep millis is ~0 so this is good for all modes
  nextSleepTime = (millis() + wake_ms); // not sleepSafeMillis() as by definition dont sleep before this

  if (mode) {
    if (mode & PauseUART) {
      frugal_iot.startSerial(); // Note turned UART off in prepare or sleep
    }
    #ifdef LILYGOHIGROW
      digitalWrite(POWER_CTRL, HIGH);
    #endif
    #ifdef SYSTEM_POWER_DEBUG
      Serial.print(F("Waking for")); Serial.println(wake_ms);
      #ifdef SYSTEM_OLED_WANT // Maybe comment out once working
        frugal_iot.oled->display.setCursor(0,40);
        frugal_iot.oled->display.print("Recovering from Sleep");  
        frugal_iot.oled->display.display();
      #endif
    #endif
    #ifdef ESP32
      // Restore comms: Power_Loop & Power_Modem do not need it; and Power_Deep does it in setup
      bool WiFiOK = true;
      if (mode & PauseWiFi) {
        WiFiOK = frugal_iot.wifi->recover();
      }
      if (WiFiOK & PauseMQTT) { // Note MQTT not being explicitly paused
        frugal_iot.mqtt->recoverFromLightSleep(); // New or old session
      }
      if (mode & DeepSleep) {
        // Memory will have been wiped by the sleep, what can we assume?
        // Note if required - can store stuff in RTC memory or on disk, but careful about overusing (writes) flash
        // Note this is called from setup() which will also call other sensors & actuators to repeat setup()
        // TODO-23 test and add a note here about order - what is this before/after ? 
        #ifdef SYSTEM_POWER_DEBUG
          Serial.print(F("Power Management: recovering from Deep Sleep: ")); Serial.println(wake_count);
        #endif
        // TODO-23 be smarter about doneFullAdvertise, for example could have powered up, failed to get comms, then slept. 
        frugal_iot.discovery->doneFullAdvertise = true; // Assume did this before sleep
      }
    #endif
  }
}

// ================== maybeSleep =========== called from main::loop() = doesnt return if deep-sleep ===
bool System_Power::maybeSleep() {
  if (nextSleepTime <= millis()) {
    #ifdef SYSTEM_POWER_DEBUG
      Serial.println(F("XXX Sleeping")); delay(1000);
    #endif
    prepare();
    sleep();
    recover(); // Never gets here in deep sleep - in that case recover is called from setup
    return true;
  }
  return false;
}

#ifdef ESP32
  // === Adjusted millis() to account for time when millis() not ticking
  unsigned long System_Power::sleepSafeMillis() {
    // Return millis() adjusted for any sleep offset
    // TODO-23 millis wraps at 49 days, so need to handle that
    return millis() + millis_offset;
  }
#endif
