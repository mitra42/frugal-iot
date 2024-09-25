/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  Based on the Blinken demo in the IDE

  Required CONTROL_BLINKEN_MS // TODO replace with a settable parameter
 */


#include "_settings.h"  // Settings for what to include etc
#include "_common.h"    // Main include file for Framework

#ifdef WANT_CONTROL_BLINKEN
#include <Arduino.h>
#include "control_blinken.h"
#include "system_clock.h"

namespace cBlinken {

unsigned long lastLoopTime = 0;

void setup() {                
}

void loop() {
  if (sClock::hasIntervalPassed(lastLoopTime, CONTROL_BLINKEN_MS)) {
    aLedbuiltin::value = !aLedbuiltin::value;
    #ifdef CONTROL_BLINKEN_DEBUG
      Serial.print("Set LED to ");
      Serial.println(aLedbuiltin::value);
    #endif // CONTROL_BLINKEN_DEBUG
    lastLoopTime = sClock::getTime();
  }
}

} //namespace cBlinken
#endif // WANT_CONTROL_BLINKEN
