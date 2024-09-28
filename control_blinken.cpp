/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  Based on the Blinken demo in the IDE

  Required CONTROL_BLINKEN_MS // TODO replace with a settable parameter
 */


#include "_settings.h"  // Settings for what to include etc
#include "_common.h"    // Main include file for Framework

#ifdef CONTROL_BLINKEN_WANT
#include <Arduino.h>
#include "control_blinken.h"
#include "system_clock.h"

namespace cBlinken {

unsigned long lastLoopTime = 0;

void setup() {                
}

void loop() {
  if (xClock::hasIntervalPassed(lastLoopTime, CONTROL_BLINKEN_MS)) {
    aLedbuiltin::value = !aLedbuiltin::value;
    #ifdef CONTROL_BLINKEN_DEBUG
      Serial.print("Set LED to ");
      Serial.println(aLedbuiltin::value);
    #endif // CONTROL_BLINKEN_DEBUG
    lastLoopTime = xClock::getTime();
  }
}

} //namespace cBlinken
#endif // CONTROL_BLINKEN_WANT
