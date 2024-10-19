/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  Based on the Blinken demo in the IDE

  Required CONTROL_BLINKEN_MS // TODO-30 replace with a settable parameter
 */


#include "_settings.h"  // Settings for what to include etc
#include "actuator_ledbuiltin.h"

#ifdef CONTROL_BLINKEN_WANT
#include <Arduino.h>
#include "_common.h"    // Main include file for Framework
#include "control_blinken.h"

namespace cBlinken {

unsigned long nextLoopTime = 0;

void setup() {                
}

void loop() {
  if (nextLoopTime <= millis()) {
    // TODO send via MQTT instead of setting directly
    aLedbuiltin::set(!aLedbuiltin::value);
    nextLoopTime = millis() + CONTROL_BLINKEN_MS;
  }
}

} //namespace cBlinken
#endif // CONTROL_BLINKEN_WANT
