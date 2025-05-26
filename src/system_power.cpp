/* Frugal IoT - System Power
 * Manage power and main loop 
 * 
 * Required: 
 *   SYSTEM_POWER_MODE_HIGH, SYSTEM_POWER_MODE_MEDIUM or SYSTEM_POWER_MODE_LOW (absence of any is (or maybe) current situation)
 */


// This will replace loop() and then parts will be put into 

#include "_settings.h"
#include "system_power.h"
#if !defined(SYSTEM_POWER_MODE_HIGH) && !defined(SYSTEM_POWER_MODE_MEDIUM) && !defined(SYSTEM_POWER_MODE_LOW)
  #define SYSTEM_POWER_MODE_HIGH // Default to high power mode if none defined
#endif
#ifndef SYSTEM_POWER_MS
  #define SYSTEM_POWER_MS 10000 // Default to 10 seconds
#endif

System_Power* powerController;

System_Power::System_Power()
: Frugal_Base() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.print("Power Management: ms="); Serial.println(SYSTEM_POWER_MS);
  #endif
}

void System_Power::setup() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println("Power Management: setup");
  #endif
}
void System_Power::prepareForDeepSleep() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println("Power Management: preparing for Deep Sleep");
  #endif

}

void System_Power::recoverFromDeepSleep() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println("Power Management: recovering from Deep Sleep");
  #endif
}
void System_Power::prepareForLightSleep() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println("Power Management: preparing for Light Sleep");
  #endif

}
void System_Power::recoverFromLightSleep() {
  #ifdef SYSTEM_POWER_DEBUG
    Serial.println("Power Management: recovering from Light Sleep");
  #endif
}

bool System_Power::maybeSleep() {
  // Note sleep is just the "period" when in SYSTEM_POWER_MODE_HIGH
  if (nextSleepTime <= millis()) {  // TODO-23 check millis is ok during each of light (probably yes)and deep (probably not) sleep
    #ifdef SYSTEM_POWER_MODE_LOW
      prepareForDeepSleep();    
      // deepsleep(SYSTEM_POWER_MS-SYSTEM_POWER_WAKE_MS)
      nextSleepTime = millis() + SYSTEM_POWER_WAKE_MS // TODO-23 check logic when know how millis works in deep sleep
      recoverFromDeepSleep();
      return true;
    #elif defined(SYSTEM_POWER_MODE_MEDIUM)
      prepareForLightSleep();    
      // lightsleep(SYSTEM_POWER_MS-SYSTEM_POWER_WAKE_MS)
      nextSleepTime = millis() + SYSTEM_POWER_WAKE_MS // TODO-23 check logic when know how millis works in light sleep
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



