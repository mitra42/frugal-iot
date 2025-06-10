/* Frugal IoT - System Power
 * Manage power and main loop 
 * 
 * Required: 
 *   SYSTEM_POWER_MODE_LOOP, SYSTEM_POWER_MODE_LIGHT or SYSTEM_POWER_MODE_DEEP (absence of any is (or maybe) current situation)
 *
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html
 */

// This will replace loop() and then parts will be put into 

#include "_settings.h"
#include "system_power.h"
#include "system_oled.h"
// TODO may not need all these once WiFi debugged and code moved to other files
#include "system_mqtt.h"

System_Power_Mode* powerController;

RTC_DATA_ATTR unsigned long wake_count = 0; // If 1 per minute, uint_16 would just be 45 days
RTC_DATA_ATTR unsigned long millis_offset = 0;  // Cumulative time in deep sleep with millis() being reset
// Note how this is placed in RTC code space so it can be executed prior to the restart
void RTC_IRAM_ATTR esp_wake_deep_sleep(void) {
    esp_default_wake_deep_sleep();
    wake_count++;
}

System_Power_Mode::System_Power_Mode(const char* name, unsigned long cycle_ms, unsigned long wake_ms)
: Frugal_Base(),
  name(name),
  cycle_ms(cycle_ms),
  wake_ms(wake_ms),
  nextSleepTime(millis() + wake_ms) // not sleepSafeMillis() as by definition dont sleep before this
{
  #ifdef SYSTEM_POWER_DEBUG
    Serial.printf("%s: %d of %d\n", name, wake_ms, cycle_ms); 
  #endif
}
// ================== constructor =========== called from main.cpp::setup based on #define ========= TO-ADD-POWERMODE
System_Power_Mode_Loop::System_Power_Mode_Loop(unsigned long cycle_ms, unsigned long wake_ms) 
: System_Power_Mode("Power Looping", cycle_ms, wake_ms) {
}
System_Power_Mode_Light::System_Power_Mode_Light(unsigned long cycle_ms, unsigned long wake_ms) 
: System_Power_Mode("Power Light Sleep",cycle_ms, wake_ms) {
}
System_Power_Mode_Deep::System_Power_Mode_Deep(unsigned long cycle_ms, unsigned long wake_ms) 
: System_Power_Mode("Power Deep Sleep", cycle_ms, wake_ms)
{
}
// TODO-23 maybe ifdef out the power modes - BUT may want to shift between them.
System_Power_Mode_Auto::System_Power_Mode_Auto(unsigned long cycle_ms, unsigned long wake_ms) 
: System_Power_Mode("Power Auto", cycle_ms, wake_ms) {
}
// ================== setup =========== called from main.cpp::setup ========= TO-ADD-POWERMODE but usually nothing
void System_Power_Mode::setup() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.printf("Setup %s: %d of %d\n", name, wake_ms, cycle_ms); 
  #endif
}
//LOW: This is not a fresh start, so we are recovering from a deep sleep (should only be in System_Power_Mode_Deep but is generically true)
void System_Power_Mode_Deep::setup() {
  if (wake_count) {
    recover(); 
  } else {
    System_Power_Mode::setup();
  }
}
// ================== prepare =========== called from maybeSleep ========= TO-ADD-POWERMODE

// prepare - run from loop (or maybeSleep) just before sleeping 
void System_Power_Mode::prepare() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println("Power Management: preparing");
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
void System_Power_Mode_Light::prepare() {
  System_Power_Mode::prepare();
  #ifdef SYSTEM_OLED_WANT
    oled->display.setCursor(0,40);
    oled->display.print("Peparing for Light Sleep");      
    oled->display.display();
  #endif
  xWifi::prepareForLightSleep(); 
}
// ================== sleep =========== called from maybeSleep ========= TO-ADD-POWERMODE
void System_Power_Mode::sleep() {
  Serial.println(F("sleep should be defined"));
}
void System_Power_Mode_Loop::sleep() {
  // Note sleep is just the "period" when in SYSTEM_POWER_MODE_LOOP
}
void System_Power_Mode_Light::sleep() {
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html#_CPPv421esp_light_sleep_startv
  esp_sleep_enable_timer_wakeup(sleep_us());
  esp_light_sleep_start();
}
void System_Power_Mode_Deep::sleep() {
  millis_offset = millis() + sleep_ms(); // Since millis() will reset to 0
  esp_deep_sleep(sleep_us());
  Serial.println(F("Power Management: failed to go into Deep Sleep - this should not happen!"));
}
void System_Power_Mode_Auto::sleep() {
}

// ================== recover =========== called from maybeSleep ========= TO-ADD-POWERMODE
// This should undo anything done in prepare - e.g. turning devices back on
// It also has to setup the nextSleepTime - which varies from mode to mode. 

void System_Power_Mode::recover() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println("Power Management: recovering");
  #endif
}

void System_Power_Mode_Loop::recover() {
  //System_Power_Mode::recover(); // Not calling superclass as nothing to recover from in Power_Mode_Loop
  nextSleepTime = millis() + cycle_ms;
}
void System_Power_Mode_Light::recover() {
  // Note millis preserved during lightSleep
  nextSleepTime = millis() + wake_ms;
  System_Power_Mode::recover();
  #ifdef SYSTEM_OLED_WANT
    oled->display.setCursor(0,40);
    oled->display.print("Recovering from Light Sleep");  
    oled->display.display();
  #endif
  if (xWifi::recoverFromLightSleep()) {
    Mqtt->recoverFromLightSleep(); // New or old session
  } 
}

void System_Power_Mode_Deep::recover() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println("Power Management: recovering from Deep Sleep: "); Serial.println(wake_count);
  #endif
  // TODO-23 maybe will loop through sensors and actuators here are some pointers found elsewhere...
  // or maybe presumes all reset by the restart after deep sleep
}
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

// === Adjusted millis() to account for time when millis() not ticking
unsigned long System_Power_Mode::sleepSafeMillis() {
  // Return millis() adjusted for any sleep offset
  // TODO-23 millis wraps at 49 days, so need to handle that
  return millis() + millis_offset;
}
