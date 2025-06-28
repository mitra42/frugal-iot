/* Frugal IoT - Watchdog
 * 
 * Can be used to monitor different internal functions - here it is watching Heap but that might change
 */

#ifdef ESP32
  #include "esp_task_wdt.h" // TODO-125
#endif
#ifdef ESP8266
#include "user_interface.h" // system_get_free_heap_size
#endif
#include "system_watchdog.h"
#include "system_frugal.h"

// TODO-125
// This pair of functions is intended to debug the freezes being seen on ESP32's 
// - see https://github.com/mitra42/frugal-iot/issues/125

#ifdef ESP32
  #define TWDT_TIMEOUT_MS 180000 // three minutes - want to be long enough.
#endif
#define SYSTEM_WATCHDOG_MEM_MS 10000 // 10 seconds, much too fast once tested 

long unsigned internal_watchdog_last = 0; // Will be sleepSafeMillis 

System_Watchdog::System_Watchdog() : System_Base("watchdog", "Watchdog") {}

void System_Watchdog::setup() {
  //Ref: https://forum.arduino.cc/t/esp32-ram-check/871248/2
  //https://iotassistant.io/esp32/enable-hardware-watchdog-timer-esp32-arduino-ide/
  // If the TWDT was not initialized automatically on startup, manually intialize it now

  #ifdef ESP32
    #ifdef PLATFORMIO // defined automatically when using PLATFORMIO in Visual Studio
      esp_task_wdt_init(TWDT_TIMEOUT_MS, true);
    #else // Assuming ARDUINO-IDE which has different definition of esp_task_wdt_init - unsure which is older, which newer
      esp_task_wdt_config_t twdt_config = {
          .timeout_ms = TWDT_TIMEOUT_MS,
          .idle_core_mask = (1 << CONFIG_FREERTOS_NUMBER_OF_CORES) - 1,    // Bitmask of all cores
          .trigger_panic = true,
      };
      esp_task_wdt_init(&twdt_config);
    #endif
    //esp_task_wdt_init(esp_task_wdt_config_t{ 3000, 0, false}); //enable panic so ESP32 restarts
    esp_task_wdt_add(NULL); //add current thread to WDT watch  
  #endif //ESP32
}

void System_Watchdog::infrequently() {
  #ifdef ESP32
    esp_task_wdt_reset();
  #endif
  // TODO move this to infrequent() on something
  if (frugal_iot.powercontroller->sleepSafeMillis() > (internal_watchdog_last + SYSTEM_WATCHDOG_MEM_MS)) {
    #ifdef SYSTEM_MEMORY_DEBUG
      #ifdef ESP8266
        Serial.print(F("heap=")); Serial.println(system_get_free_heap_size());  // https://www.esp8266.com/viewtopic.php?p=82839
      #elif defined(ESP32) //TODO-128 should be able to find equivalent on ESP8266
        Serial.print(F("heap=")); Serial.print(esp_get_free_heap_size()); 
        Serial.print(F(" min heap=")); Serial.println(esp_get_minimum_free_heap_size());
      #endif //ESP32
    #endif
    internal_watchdog_last = frugal_iot.powercontroller->sleepSafeMillis(); 
  }
}
