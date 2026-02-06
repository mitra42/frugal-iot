/* Frugal IoT - Watchdog
 * 
 * Can be used to monitor different internal functions - here it is watching Heap but that might change
 * 
 * NOTE - this is not currently included anywhere - can add to System_Frugal like other system classes
 */

#ifdef ESP32
  #include "esp_task_wdt.h" // TODO-125
#endif
#ifdef ESP8266
#include "user_interface.h" // system_get_free_heap_size
#endif
#include "system_watchdog.h"
#include "system_frugal.h"
#include "misc.h" // heap_print


// TODO-125
// This pair of functions is intended to debug the freezes being seen on ESP32's 
// - see https://github.com/mitra42/frugal-iot/issues/125

#ifdef ESP32
  #define TWDT_TIMEOUT_MS (180000) // three minutes - want to be long enough.
#endif
#define SYSTEM_WATCHDOG_MEM_MS 10000 // 10 seconds, much too fast once tested 

System_Watchdog::System_Watchdog() 
: System_Base("watchdog", "Watchdog"),
  timer_index(frugal_iot.powercontroller->timer_next())
 {}

void System_Watchdog::setup() {
    // Nothing to read from disk so not calling readConfigFromFS 
    
  //Ref: https://forum.arduino.cc/t/esp32-ram-check/871248/2
  //https://iotassistant.io/esp32/enable-hardware-watchdog-timer-esp32-arduino-ide/
  // If the TWDT was not initialized automatically on startup, manually intialize it now

  #ifdef ESP32
   #ifdef OBSOLETE_PLATFORMIO // defined automatically when using PLATFORMIO in Visual Studio
      esp_task_wdt_init(TWDT_TIMEOUT_MS, true);
   #else // Assuming ARDUINO-IDE which has different definition of esp_task_wdt_init - unsure which is older, which newer
      esp_task_wdt_config_t twdt_config = {
          .timeout_ms = TWDT_TIMEOUT_MS,
          .idle_core_mask = (1 << CONFIG_FREERTOS_NUMBER_OF_CORES) - 1,    // Bitmask of all cores
          .trigger_panic = true,
      };
      esp_task_wdt_reconfigure(&twdt_config); // Was esp_task_wdt_init 
   #endif
    //esp_task_wdt_init(esp_task_wdt_config_t{ 3000, 0, false}); //enable panic so ESP32 restarts
    esp_task_wdt_add(NULL); //add current thread to WDT watch  
  #endif //ESP32
}

void System_Watchdog::loop() {
  static uint16_t watchcount = 0;
  if (watchcount++ >= 10000 ) {
    watchcount = 0;
    Serial.print(F("⏳"));
  }
  #ifdef ESP32
    esp_task_wdt_reset();
  #endif
}

#if defined(SYSTEM_MEMORY_DEBUG)
void System_Watchdog::infrequently() {
  if (frugal_iot.powercontroller->timer_expired(timer_index)) { // 
    #ifdef SYSTEM_MEMORY_DEBUG
      //heap_print(F("watchdog infrequent"));
    #endif
    frugal_iot.powercontroller->timer_set(timer_index, SYSTEM_WATCHDOG_MEM_MS);
  }
}
#endif