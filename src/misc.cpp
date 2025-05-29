
// Some common functions - ifdef them for the functions that use them
#include <Arduino.h>
#include "_settings.h"
#ifdef ESP32
  #include "esp_task_wdt.h" // TODO-125
#endif
#ifdef ESP8266
#include "user_interface.h" // system_get_free_heap_size
#endif
#include "system_power.h" // For sleepSafemillis()

#include <Arduino.h> // For String
//#include <stdio.h> // Doesnt appear to be needed - was in sample code from Jonathan Semple
//#include <stdarg.h> // Doesnt appear to be needed - was in sample code from Jonathan Semple
//#include "WString.h" // Doesnt appear to be needed - was in sample code from Jonathan Semple

const String StringF(const char* format, ...) {
    char buffer[200]; // out of scope at end of this
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    return String(buffer); // Note - string returned on stack so should be safe
}

// Typical usage.   lprintf(strlen(a)+strlen(b)+2, "%s %s", a, b) note how add 1 for length
const char* lprintf(size_t buffer_size, const char* format, ...) {
  // Be careful with this, there is no compile time checking that the number of args matches the format 
  // and a mismatch will generate an Exception
  char* buffer = new char[buffer_size];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, buffer_size, format, args);
  return buffer; // This buffer should stay in scope - and must be explicitly freed up by delete() if not wanted.
}

// TODO-125
// This pair of functions is intended to debug the freezes being seen on ESP32's 
// - see https://github.com/mitra42/frugal-iot/issues/125

#ifdef ESP32
  #define TWDT_TIMEOUT_MS 180000 // three minutes - want to be long enough.
#endif
#define SYSTEM_WATCHDOG_MEM_MS 10000 // 10 seconds, much too fast once tested 

long unsigned internal_watchdog_last = 0; // Will be sleepSafeMillis 

void internal_watchdog_setup() {
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

void internal_watchdog_loop() {
  #ifdef ESP32
    esp_task_wdt_reset();
  #endif
  if (powerController->sleepSafeMillis() > (internal_watchdog_last + SYSTEM_WATCHDOG_MEM_MS)) {
    #ifdef ESP8266
      Serial.print(F("heap=")); Serial.println(system_get_free_heap_size());  // https://www.esp8266.com/viewtopic.php?p=82839
    #elif defined(ESP32) //TODO-128 should be able to find equivalent on ESP8266
      Serial.print(F("heap=")); Serial.print(esp_get_free_heap_size()); 
      Serial.print(F(" min heap=")); Serial.println(esp_get_minimum_free_heap_size());
    #endif //ESP32
    internal_watchdog_last = powerController->sleepSafeMillis(); 
  }
}
