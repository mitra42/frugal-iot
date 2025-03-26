/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  Based on the Blinken demo in the IDE

  Required CONTROL_BLINKEN_S // Initial time, note overridden by message

  TODO-43 - allow the destination to be set - e.g. some other digital pin
  TODO-43 - allow the input to be routed from somewhere (e.g. from temperature or a potentiometer )

 */

#include "_settings.h"  // Settings for what to include etc

#ifdef CONTROL_BLINKEN_WANT

#if (!defined(CONTROL_BLINKEN_S))
  // Setting default blink of 1 second, override in _local.h. 
  #define CONTROL_BLINKEN_S 1
#endif

#include <Arduino.h>
#include "control_blinken.h"
#include "control.h"
#include "misc.h" // for lprintf

ControlBlinken::ControlBlinken (const char* const name, float blinkSpeed) : Control(
  lprintf(strlen(name)+9, "%s_control", name),
  std::vector<IN*> {
    new INfloat(lprintf(strlen(name)+6, "%s speed", name), blinkSpeed, lprintf(strlen(name)+7, "%s_speed", name), 0, 3600, "black", true),
  },
  std::vector<OUT*> {
    new OUTbool(lprintf(strlen(name)+4, "%s Out", name), false, lprintf(strlen(name)+5, "%s_out", name), "black", true), 
  }
), blinkSpeed(blinkSpeed * 1000) {
  #ifdef CONTROL_BLINKEN_DEBUG
    debug("ControlBlinken after instantiation");
  #endif
};

void ControlBlinken::act() {
  // If calling act, then we know blinkSpeed changed
  blinkSpeed = inputs[0]->floatValue() * 1000;
  nextBlinkTime = millis() + blinkSpeed; // Blink after new blink time
}

void ControlBlinken::loop() {
  if (nextBlinkTime <= millis()) {
    ((OUTbool*)outputs[0])->set(!outputs[0]->boolValue()); // Will send message
    nextBlinkTime = millis() + blinkSpeed;
  }
}
#endif // CONTROL_BLINKEN_WANT
