/* Frugal IoT - System Power
 * Manage power and main loop 
 * 
 * Required: 
 *   SYSTEM_POWER_MODE_HIGH, SYSTEM_POWER_MODE_MEDIUM or SYSTEM_POWER_MODE_LOW (absence of any is (or maybe) current situation)
 *
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html
 */

// This will replace loop() and then parts will be put into 

#include "_settings.h"
#include "system_power.h"
#include "system_oled.h"
// TODO may not need all these once WiFi debugged and code moved to other files
#include "system_mqtt.h"

System_Power* powerController;

RTC_DATA_ATTR unsigned long wake_count = 0; // If 1 per minute, uint_16 would just be 45 days
RTC_DATA_ATTR unsigned long millis_offset = 0; 

System_Power::System_Power()
: Frugal_Base(),
  nextSleepTime(millis() + SYSTEM_POWER_WAKE_MS) // not sleepSafeMillis() as by definition dont sleep before this
{
  #ifdef SYSTEM_POWER_DEBUG
    Serial.print("Power Management: "); 
    #ifdef SYSTEM_POWER_MODE_HIGH
      Serial.print("High Power Mode");
    #elif defined(SYSTEM_POWER_MODE_MEDIUM)
      Serial.print("Medium Power Mode");
    #else
      Serial.print("Low Power Mode");
    #endif
    Serial.print("period="); Serial.print(SYSTEM_POWER_MS); 
    Serial.print(" wake="); Serial.println(SYSTEM_POWER_WAKE_MS);
  #endif
}

void System_Power::setup() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println("Power Management: setup");
  #endif
  if (wake_count) {
    // This is not a fresh start, so we are recovering from a deep sleep
    recoverFromDeepSleep();
  }
}
void System_Power::prepareForDeepSleep() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println("Power Management: preparing for Deep Sleep");
  #endif

}

void RTC_IRAM_ATTR esp_wake_deep_sleep(void) {
    esp_default_wake_deep_sleep();
    wake_count++;
}
void System_Power::recoverFromDeepSleep() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println("Power Management: recovering from Deep Sleep: "); Serial.println(wake_count);
  #endif
}
void System_Power::prepareForLightSleep() {
  #ifdef SYSTEM_OLED_WANT
    oled->display.setCursor(0,40);
    oled->display.print("Peparing for Light Sleep");      
    oled->display.display();
  #endif
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println("Power Management: preparing for Light Sleep");
    // Serial.print("Millis ="); Serial.println(millis()); // Tested and confirms preserved
  #endif
  xWifi::prepareForLightSleep(); 
  // TODO-23 will loop through sensors and actuators here are some pointers found elsewhere...
  // WIFI 
  // see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/power_management.html for how to keep WiFi alive during sleep
  // esp_wifi_stop() // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html
  // WiFi.mode(WIFI_MODE_NULL);
  // LoRa.sleep();
  // SPI.end();
}
void System_Power::recoverFromLightSleep() {
  // Just used for status loops
  uint8_t wait_seconds = 10; // TODO Should be less
  unsigned long starttime = millis(); // not sleepSafeMillis - and thats probably ok

  #ifdef SYSTEM_OLED_WANT
    oled->display.setCursor(0,40);
    oled->display.print("Recovering from Light Sleep");  
    oled->display.display();
  #endif
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println("Power Management: recovering from Light Sleep");
    // Serial.print("Millis ="); Serial.println(millis());   // Tested and confirned preserved
  #endif
  if (xWifi::recoverFromLightSleep()) {
    Mqtt->recoverFromLightSleep(); // New or old session
  } 
  // TODO-23 will loop through sensors and actuators here are some pointers found elsewhere...
}

bool System_Power::maybeSleep() {
  // TODO-23 handle offset for millis() 
  // Note sleep is just the "period" when in SYSTEM_POWER_MODE_HIGH
  if (nextSleepTime <= millis()) {  
  
    #ifdef SYSTEM_POWER_MODE_LOW
      prepareForDeepSleep(); 
      delay(1000); // Not sure why, found in LilyGo code
      Serial.println(F("Power Management: going to Deep Sleep"));
      millis_offset = millis() + SYSTEM_POWER_SLEEP_MS; // Since millis() will reset to 0
      esp_deep_sleep(SYSTEM_POWER_SLEEP_US);
      // TODO-23 handle offset for millis() 
      Serial.println(F("Power Management: failed to go into Deep Sleep - this should not happen!"));
      // It doesnt come back from deep sleep - can define esp_wake_deep_sleep() to do something on wakeup
      // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html#_CPPv419esp_wake_deep_sleepv
      // But note need to put in RTC memory - see https://mischianti.org/esp32-practical-power-saving-store-data-timer-and-touch-wake-up-4/ and RTC_IRAM_ATTR
      return true;
    #elif defined(SYSTEM_POWER_MODE_MEDIUM)
      prepareForLightSleep();
      // esp_light_sleep_start() // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html#_CPPv421esp_light_sleep_startv
      // lightsleep(SYSTEM_POWER_MS-SYSTEM_POWER_WAKE_MS)
      esp_sleep_enable_timer_wakeup(SYSTEM_POWER_SLEEP_US);
      esp_light_sleep_start();
      nextSleepTime = millis() + SYSTEM_POWER_WAKE_MS; // TODO-23 check logic when know how millis works in light sleep
      // Note millis preserved during lightSleep
      recoverFromLightSleep();
      return true;
    #elif defined(SYSTEM_POWER_MODE_HIGH)
      nextSleepTime = millis() + SYSTEM_POWER_MS;
      return true; 
    #else
      #error "Must define one of SYSTEM_POWER_MODE_HIGH _MEDIUM or _LOW"
    #endif
  }
  return false;
}

unsigned long System_Power::sleepSafeMillis() {
  // Return millis() adjusted for any sleep offset
  // TODO-23 millis wraps at 49 days, so need to handle that
  return millis() + millis_offset;
}
