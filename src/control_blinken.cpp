/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  Based on the Blinken demo in the IDE

  TODO-43 - allow the destination to be set - e.g. some other digital pin
  TODO-43 - allow the input to be routed from somewhere (e.g. from temperature or a potentiometer )

 */

#include "_settings.h"  // Settings for what to include etc

#ifdef CONTROL_BLINKEN_WANT

#include <Arduino.h>
#include "control_blinken.h"
#include "control.h"
#include "misc.h" // for lprintf

ControlBlinken::ControlBlinken (const char* const name, float secsOn, float secsOff) : Control(
  lprintf(strlen(name)+9, "%s_control", name),
  std::vector<IN*> {
    new INfloat(lprintf(strlen(name)+9, "%s time on", name), secsOn, lprintf(strlen(name)+4, "%s_on", name), 0, 3600, "black", true),
    new INfloat(lprintf(strlen(name)+10, "%s time off", name), secsOff, lprintf(strlen(name)+5, "%s_off", name), 0, 3600, "black", true),
  },
  std::vector<OUT*> {
    new OUTbool(lprintf(strlen(name)+5, "%s Out", name), false, lprintf(strlen(name)+5, "%s_out", name), "black", true), 
  }
), blinkOn(secsOn * 1000), blinkOff(secsOff * 1000) {
  #ifdef CONTROL_BLINKEN_DEBUG
    debug("ControlBlinken after instantiation");
    Serial.print(F("ControlBlinken: blinkOn: ")); Serial.print(blinkOn); Serial.print(F(" blinkOff: ")); Serial.println(blinkOff);
  #endif
};

void ControlBlinken::act() {
  // If calling act, then we know blinkSpeed changed
  blinkOn = inputs[0]->floatValue() * 1000;
  blinkOff = inputs[1]->floatValue() * 1000;
  nextBlinkTime = millis() + (outputs[0]->boolValue() ? blinkOn : blinkOff) ; // Blink after new blink time
}

void ControlBlinken::loop() {
  if (nextBlinkTime <= millis()) {
    Serial.print("XXX "); Serial.print(__FILE__); Serial.println(__LINE__);
    bool next = !outputs[0]->boolValue();
    ((OUTbool*)outputs[0])->set(next); // Will send message
    nextBlinkTime = millis() + (next ? blinkOn : blinkOff);
  }
}
#endif // CONTROL_BLINKEN_WANT
