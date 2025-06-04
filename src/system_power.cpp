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

System_Power_Mode* powerController;


RTC_DATA_ATTR unsigned long wake_count = 0; // If 1 per minute, uint_16 would just be 45 days
RTC_DATA_ATTR unsigned long millis_offset = 0; 

System_Power_Mode::System_Power_Mode(unsigned long cycle_ms, unsigned_long wake_ms)
: Frugal_Base(),
  cycle_ms(cycle_ms),
  wake_ms(wake_ms),
  nextSleepTime(millis() + wake_ms) // not sleepSafeMillis() as by definition dont sleep before this
{ }
System_Power_Mode_High::System_Power_Mode_High() : System_Power_Mode() {
  #ifdef SYSTEM_POWER_DEBUG
      Serial.printf("Power Management Mode High: %d of %d\n"); 
}
System_Power_Mode_Medium::System_Power_Mode_Medium() : System_Power_Mode() {
  #ifdef SYSTEM_POWER_DEBUG
      Serial.printf("Power Management Mode Medium: %d of %d\n"); 
  #endif
}
System_Power_Mode_Low::System_Power_Mode_Low() : System_Power_Mode() {
  #ifdef SYSTEM_POWER_DEBUG
      Serial.printf("Power Management Mode Low: %d of %d\n"); 
  #endif
}
System_Power_Mode_Auto::System_Power_Mode_Auto() : System_Power_Mode() {
  #ifdef SYSTEM_POWER_DEBUG
      Serial.printf("Power Management Mode Auto: %d of %d\n"); 
  #endif
}



void System_Power::setup() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println("Power Management: setup");
  #endif
}
void System_Power_Low::setup() {
  if (wake_count) {
    // This is not a fresh start, so we are recovering from a deep sleep
    recoverFromSleep(); 
  } else {
    System_Power_Mode::setup();
  }
}
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
void System_Power_Medium::prepare() {
  System_Power_Mode::prepare();
  #ifdef SYSTEM_OLED_WANT
    oled->display.setCursor(0,40);
    oled->display.print("Peparing for Light Sleep");      
    oled->display.display();
  #endif
  xWifi::prepareForLightSleep(); 
}
System_Power_Mode_High::sleep() {
  // Note sleep is just the "period" when in SYSTEM_POWER_MODE_HIGH
}
System_Power_Mode_Medium::sleep() {
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html#_CPPv421esp_light_sleep_startv
  esp_sleep_enable_timer_wakeup(sleep_us);
  esp_light_sleep_start();
}
System_Power_Mode_Low::sleep() {
  millis_offset = millis() + sleep_ms; // Since millis() will reset to 0
  esp_deep_sleep(sleep_us);
  Serial.println(F("Power Management: failed to go into Deep Sleep - this should not happen!"));
}
System_Power_Mode_Auto::sleep() {
}

// Note how this is placed in RTC code space so it can be executed prior to the restart
void RTC_IRAM_ATTR esp_wake_deep_sleep(void) {
    esp_default_wake_deep_sleep();
    wake_count++;
}
void System_Power_Mode::recover() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println("Power Management: recovering");
  #endif
}

void System_Power_High::recover() {
      nextSleepTime = millis() + cycle_ms;
}

void System_Power_Mode_Medium::recover() {
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

void System_Power_Low::recover() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println("Power Management: recovering from Deep Sleep: "); Serial.println(wake_count);
  #endif
  // TODO-23 will loop through sensors and actuators here are some pointers found elsewhere...
}

// maybeSleep is the API that should be used in main.cpp.loop() 
bool System_Power_Mode::maybeSleep() {
  if (nextSleepTime <= millis()) {
    prepare();
    sleep();
    recover(); // Never gets here in deep sleep - in that case recover is called from setup
    return true;
  }
  return false;
}

unsigned long System_Power::sleepSafeMillis() {
  // Return millis() adjusted for any sleep offset
  // TODO-23 millis wraps at 49 days, so need to handle that
  return millis() + millis_offset;
}


#ifdef SYSTEM_POWER_HIGH
  System_Power_Mode_High* powerController;
#elif defined(SYSTEM_POWER_MODE_MEDIUM)
  System_Power_Mode_Medium* powerController(SYSTEM_POWER_MS, SYSTEM_POWER_WAKE);
#elif defined(SYSTEM_POWER_MODE_LOW)
  System_Power_Mode_Low* powerController;
#elif defined(SYSTEM_POWER_MODE_Auto)
  System_Power_Mode_Auto powerController;
#else
  #error Must define valid power mode
#endif
