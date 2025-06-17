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
#include "frugal_iot.h"

ControlBlinken::ControlBlinken (const char* const id, const char * const name, float secsOn, float secsOff) 
: Control(id, name,
  std::vector<IN*> {
    new INfloat(id, "timeon", "Time On", secsOn, 3, 0, 3600, "black", true),
    new INfloat(id, "timeoff", "Time Off", secsOff, 3, 0, 3600, "black", true),
  },
  std::vector<OUT*> {
    new OUTbool(id, "out", name, false, "black", true), 
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
  nextBlinkTime = frugal_iot.powercontroller->sleepSafeMillis() + (outputs[0]->boolValue() ? blinkOn : blinkOff) ; // Blink after new blink time
}

void ControlBlinken::frequently() {
  if (nextBlinkTime <= frugal_iot.powercontroller->sleepSafeMillis()) {
    bool next = !outputs[0]->boolValue();
    ((OUTbool*)outputs[0])->set(next); // Will send message
    nextBlinkTime = frugal_iot.powercontroller->sleepSafeMillis() + (next ? blinkOn : blinkOff);
  }
}
#endif // CONTROL_BLINKEN_WANT
