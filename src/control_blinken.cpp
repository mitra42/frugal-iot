/* Frugal IoT - Blinken demo, a simple control that blinks a light
 *
 * Optional: CONTROL_BLINKEN_DEBUG
 */

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "control_blinken.h"
#include "control.h"
#include "system_frugal.h"

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

void ControlBlinken::loop() {
  if (nextBlinkTime <= frugal_iot.powercontroller->sleepSafeMillis()) {
    bool next = !outputs[0]->boolValue();
    ((OUTbool*)outputs[0])->set(next); // Will send message
    nextBlinkTime = frugal_iot.powercontroller->sleepSafeMillis() + (next ? blinkOn : blinkOff);
  }
}
