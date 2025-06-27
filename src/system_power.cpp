/* Frugal IoT - System Power
 * Manage power and main loop 
 * 
 * Required: 
 *   SYSTEM_POWER_MODE_LOOP, 
 *   SYSTEM_POWER_MODE_XXX is used by main.cpp (currently) not here.
 * 
 *  List of power modes ... TO-ADD-POWERMODE
 * SYSTEM_POWER_MODE_LOOP     Standard loop, no waiting
 * SYSTEM_POWER_MODE_LIGHT    Does a Light sleep
 * SYSTEM_POWER_MODE_LIGHT_WIFI Like Light, but wakes on Wifi, which menas it SHOULD keep WiFi alive.
 * SYSTEM_POWER_MODE_MODEM    ESP32 Modem sleep mode - need to check waht this means
 * SYSTEM_POWER_MODE_DEEP     Does a deep sleep - resulting in a restart
 *
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html
 */

// This will replace loop() and then parts will be put into 

#include <Arduino.h> // Required to get sdkconfig.h to get CONFIG_IDF_TARGET_xxx
// TODO may not need all these once WiFi debugged and code moved to other files
//#include "system_frugal.h"
#ifdef ESP32 // Not available on ESP8266 - have not yet searched for equivalents
  // Next three .h might or might not be needed 
  #include "esp_pm.h"
  #include "esp_wifi.h"
  #include "esp_sleep.h"
  #include "driver/uart.h"   // To allow disabling UART
  // To allow printing task list
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
#endif
#include "_settings.h"
#include "system_power.h"
#include "system_frugal.h"

/*
// Wont work in Arduino framework 
void printTaskList() {
  char buf[1024];
  vTaskList(buf);
  Serial.println("Task Name\tStatus\tPrio\tStack\tNum");
  Serial.println(buf);
}
*/

System_Power_Mode* powerController;

#ifdef ESP32
  RTC_DATA_ATTR unsigned long wake_count = 0; // If 1 per minute, uint_16 would just be 45 days
  RTC_DATA_ATTR unsigned long millis_offset = 0;  // Cumulative time in deep sleep with millis() being reset
#endif
#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
// Note how this is placed in RTC code space so it can be executed prior to the restart
void RTC_IRAM_ATTR esp_wake_deep_sleep(void) {
    esp_default_wake_deep_sleep();
    wake_count++;
}
#endif

System_Power_Mode::System_Power_Mode(const char* name, unsigned long cycle_ms, unsigned long wake_ms)
: System_Base("power", name),
  nextSleepTime(millis() + wake_ms), // not sleepSafeMillis() as by definition dont sleep before this
  cycle_ms(cycle_ms),
  wake_ms(wake_ms)
{
  #ifdef SYSTEM_POWER_DEBUG
    Serial.printf("%s: %lu of %lu\n", name, wake_ms, cycle_ms); 
  #endif
}
// ================== constructor =========== called from main.cpp::setup based on #define ========= TO-ADD-POWERMODE
System_Power_Mode_Loop::System_Power_Mode_Loop(unsigned long cycle_ms, unsigned long wake_ms) 
: System_Power_Mode("Power Looping", cycle_ms, wake_ms) {
}
#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
System_Power_Mode_Light::System_Power_Mode_Light(unsigned long cycle_ms, unsigned long wake_ms) 
: System_Power_Mode("Power Light Sleep",cycle_ms, wake_ms) {
}
System_Power_Mode_Deep::System_Power_Mode_Deep(unsigned long cycle_ms, unsigned long wake_ms) 
: System_Power_Mode("Power Deep Sleep", cycle_ms, wake_ms)
{ }
// TODO-23 maybe ifdef out the power modes - BUT may want to shift between them.
System_Power_Mode_LightWifi::System_Power_Mode_LightWifi(unsigned long cycle_ms, unsigned long wake_ms) 
: System_Power_Mode("Power Light Sleep + Wifi", cycle_ms, wake_ms) {
}
System_Power_Mode_Modem::System_Power_Mode_Modem(unsigned long cycle_ms, unsigned long wake_ms) 
: System_Power_Mode("Power Modem sleep", cycle_ms, wake_ms) {
}
#endif

// ================== setup =========== called from main.cpp::setup ========= TO-ADD-POWERMODE but usually nothing
void System_Power_Mode::setup() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.printf("Setup %s: %lu of %lu\n", name, wake_ms, cycle_ms); 
  #endif
  #ifdef LILYGOHIGROW
    pinMode(POWER_CTRL, OUTPUT);
    digitalWrite(POWER_CTRL, HIGH); // TODO-115 this is for power control - may need other board specific stuff somewhere
  #endif
}
#ifdef ESP32 // Specific to ESP32s
//LOW: This is not a fresh start, so we are recovering from a deep sleep (should only be in System_Power_Mode_Deep but is generically true)
void System_Power_Mode_Deep::setup() {
  if (wake_count) {
    recover(); 
  } else {
    System_Power_Mode::setup();
  }
}
#endif

#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
void System_Power_Mode_LightWifi::setup() {
  // This bit is weird - there are 5 different ESP32 config structures - all identical - note CONFIG_IDF_TARGET_ESP32xx is defined in board files
  #if defined(CONFIG_IDF_TARGET_ESP32C3) // Defined in board files on PlatformIO untested on Arduino
    esp_pm_config_esp32c3_t pm_config; // Seems identical structure to the default ESP32 one ! 
  #elif defined(CONFIG_IDF_TARGET_ESP32S2)
    esp_pm_config_esp32s2_t pm_config;
  #elif defined(CONFIG_IDF_TARGET_ESP32S3)
    esp_pm_config_esp32s3_t pm_config;
  #elif defined(CONFIG_IDF_TARGET_ESP32H2)
    esp_pm_config_esp32h2_t pm_config;
  #else
    esp_pm_config_esp32_t pm_config;
  #endif
  pm_config.max_freq_mhz = 240;
  pm_config.min_freq_mhz = 80;
  pm_config.light_sleep_enable = true;
  esp_pm_configure(&pm_config);   // Automatic light sleep when CPU idle
  esp_log_level_set("*", ESP_LOG_DEBUG);

  // Enable modem sleep (WiFi will automatically enter modem sleep when possible)
  esp_wifi_set_ps(WIFI_PS_MIN_MODEM); // or WIFI_PS_MAX_MODEM for more savings
  System_Power_Mode::setup();
}
#endif

// ================== prepare =========== called from maybeSleep ========= TO-ADD-POWERMODE

// prepare - run from loop (or maybeSleep) just before sleeping 
void System_Power_Mode::prepare() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println("Power Management: preparing");
  #endif
  #ifdef LILYGOHIGROW
    digitalWrite(POWER_CTRL, LOW);
  #endif

  // TODO-23 will loop through sensors and actuators here are some pointers found elsewhere...
  // WIFI 
  // see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/power_management.html for how to keep WiFi alive during sleep
  // esp_wifi_stop() // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html
  // WiFi.mode(WIFI_MODE_NULL);
  // LoRa.sleep();
  // SPI.end();
}
void System_Power_Mode_Loop::prepare() {
    //System_Power_Mode::prepare(); // Not calling superclass as nothing to prepare for in Power_Mode_Loop
}
#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
void System_Power_Mode_Light::prepare() {
  System_Power_Mode::prepare();
  frugal_iot.wifi->prepareForLightSleep(); 
}
#endif
// ================== sleep =========== called from maybeSleep ========= TO-ADD-POWERMODE
void System_Power_Mode::sleep() {
  Serial.println(F("sleep should be defined"));
}
void System_Power_Mode_Loop::sleep() {
  // Note sleep is just the "period" when in SYSTEM_POWER_MODE_LOOP
}
#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
void System_Power_Mode_Light::sleep() {
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html#_CPPv421esp_light_sleep_startv
  esp_sleep_enable_timer_wakeup(sleep_us());
  esp_light_sleep_start();
}
#endif
// This one doesn't seem to work. Power consumption doesn't go down - looks like not entering automatic lightsleep
// Copilot thinks a task is keeping it awake but vTaskList wont work in framework:arduino
#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
void System_Power_Mode_LightWifi::sleep() {
  //esp_sleep_enable_wifi_wakeup
  Serial.print("Sleeping for "); Serial.println(sleep_ms());
  // TODO-25 move this to prepare
  frugal_iot.mqtt->client.disconnect();
  //printTaskList(); // Wont work in Arduino framework
  uart_driver_delete(UART_NUM_0); // Disable UART0 (Serial)
  delay(sleep_ms()); // Light sleep will be automatic
  // TODO-25 move this to recover
  Serial.begin(SERIAL_BAUD);
  Serial.print("Waking for"); Serial.println(wake_ms);
}
#endif
#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
void System_Power_Mode_Modem::sleep() {
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html#_CPPv421esp_light_sleep_startv
  esp_sleep_enable_timer_wakeup(sleep_us());
  esp_sleep_enable_wifi_wakeup();
  esp_light_sleep_start();
}
#endif
#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
void System_Power_Mode_Deep::sleep() {
  millis_offset = millis() + sleep_ms(); // Since millis() will reset to 0
  esp_deep_sleep(sleep_us());
  Serial.println(F("Power Management: failed to go into Deep Sleep - this should not happen!"));
}
#endif
// ================== recover =========== called from maybeSleep ========= TO-ADD-POWERMODE
// This should undo anything done in prepare - e.g. turning devices back on
// It also has to setup the nextSleepTime - which varies from mode to mode. 

void System_Power_Mode::recover() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println("Power Management: recovering");
  #endif
  #ifdef LILYGOHIGROW
    digitalWrite(POWER_CTRL, HIGH);
  #endif
}

void System_Power_Mode_Loop::recover() {
  //System_Power_Mode::recover(); // Not calling superclass as nothing to recover from in Power_Mode_Loop
  nextSleepTime = millis() + cycle_ms;
}
#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
void System_Power_Mode_Light::recover() {
  // Note millis preserved during lightSleep
  nextSleepTime = millis() + wake_ms;
  System_Power_Mode::recover();
  #ifdef SYSTEM_OLED_WANT
    frugal_iot.oled->display.setCursor(0,40);
    frugal_iot.oled->display.print("Recovering from Light Sleep");  
    frugal_iot.oled->display.display();
  #endif
  if (frugal_iot.wifi->recoverFromLightSleep()) {
    frugal_iot.mqtt->recoverFromLightSleep(); // New or old session
  } 
}
#endif
#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
void System_Power_Mode_LightWifi::recover() {
  // Note millis preserved during automatic lightSleep
  nextSleepTime = millis() + wake_ms;
  System_Power_Mode::recover();
  #ifdef SYSTEM_OLED_WANT
    frugal_iot.oled->display.setCursor(0,40);
    frugal_iot.oled->display.print("Recovering from Light Sleep");  
    frugal_iot.oled->display.display();
  #endif
  //if (frugal_iot.wifi->recoverFromLightSleep()) {
    frugal_iot.mqtt->recoverFromLightSleep(); // New or old session
  //} 
}
#endif
#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
void System_Power_Mode_Deep::recover() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println("Power Management: recovering from Deep Sleep: "); Serial.println(wake_count);
  #endif
  // TODO-23 maybe will loop through sensors and actuators here are some pointers found elsewhere...
  // or maybe presumes all reset by the restart after deep sleep
}
#endif
// ================== maybeSleep =========== called from main::loop() = doesnt return if deep-sleep ===
bool System_Power_Mode::maybeSleep() {
  if (nextSleepTime <= millis()) {
    prepare();
    sleep();
    recover(); // Never gets here in deep sleep - in that case recover is called from setup
    return true;
  }
  return false;
}

#ifdef ESP32
  // === Adjusted millis() to account for time when millis() not ticking
  unsigned long System_Power_Mode::sleepSafeMillis() {
    // Return millis() adjusted for any sleep offset
    // TODO-23 millis wraps at 49 days, so need to handle that
    return millis() + millis_offset;
  }
#endif
