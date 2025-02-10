
// Some common functions - ifdef them for the functions that use them
#include <Arduino.h>
#include "_settings.h"
//TODO-125 not sure if any of this needed
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// end of unsure
#include "esp_task_wdt.h" // TODO-125

#if defined(SYSTEM_TIME_WANT) || defined(CONTROL_WANT)
#include <Arduino.h> // For String
//#include <stdio.h> // Doesnt appear to be needed - was in sample code from Jonathan Semple
//#include <stdarg.h> // Doesnt appear to be needed - was in sample code from Jonathan Semple
//#include "WString.h" // Doesnt appear to be needed - was in sample code from Jonathan Semple

const String StringF(const char* format, ...) {
    char buffer[200];

    if (! format) {
        sprintf(buffer, "Missing format string");
    } else {
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
    } 
    return String(buffer);
}
#endif 

#ifdef ESP32
// TODO-125
// This pair of functions is intended to debug the freezes being seen on ESP32's 
// - see https://github.com/mitra42/frugal-iot/issues/125

#define TWDT_TIMEOUT_MS 3000 // three minutes - want to be long enough.
#define SYSTEM_WATCHDOG_MEM_MS 10000 // 10 seconds, much too fast once tested 

long unsigned internal_watchdog_last = 0;

void internal_watchdog_setup() {
  //Ref: https://forum.arduino.cc/t/esp32-ram-check/871248/2
  //https://iotassistant.io/esp32/enable-hardware-watchdog-timer-esp32-arduino-ide/
  // If the TWDT was not initialized automatically on startup, manually intialize it now
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = TWDT_TIMEOUT_MS,
        .idle_core_mask = (1 << CONFIG_FREERTOS_NUMBER_OF_CORES) - 1,    // Bitmask of all cores
        .trigger_panic = true,
    };
    esp_task_wdt_init(&twdt_config);
  //esp_task_wdt_init(esp_task_wdt_config_t{ 3000, 0, false}); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch  
}

void internal_watchdog_loop() {
  esp_task_wdt_reset();
  if (millis() > (internal_watchdog_last + SYSTEM_WATCHDOG_MEM_MS)) {
    Serial.print("XXX125 heap="); Serial.print(esp_get_free_heap_size()); 
    Serial.print(" min heap="); Serial.println(esp_get_minimum_free_heap_size());
    internal_watchdog_last = millis(); 
  }
}
#endif //ESP32

