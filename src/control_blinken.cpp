/* Frugal IoT - Blinken demo, a simple control that blinks a light
 *
 * Optional: CONTROL_BLINKEN_DEBUG
 */

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "control_blinken.h"
#include "control.h"
#include "system_frugal.h"

Control_Blinken::Control_Blinken (const char* const id, const char * const name, float secsOn, float secsOff) 
: Control(id, name,
  std::vector<IN*> {
    new INfloat(id, "timeon", "Time On (s)", secsOn, 3, 0, 3600, "black", true),
    new INfloat(id, "timeoff", "Time Off (s)", secsOff, 3, 0, 3600, "black", true),
  },
  std::vector<OUT*> {
    new OUTbool(id, "out", name, false, "black", true), 
  }
), blinkOn(secsOn * 1000), blinkOff(secsOff * 1000) {
  #ifdef CONTROL_BLINKEN_DEBUG
    debug("Control_Blinken after instantiation");
    Serial.print(F("Control_Blinken: blinkOn: ")); Serial.print(blinkOn); Serial.print(F(" blinkOff: ")); Serial.println(blinkOff);
  #endif
};

void Control_Blinken::act() {
  // If calling act, then we know blinkSpeed changed
  blinkOn = inputs[0]->floatValue() * 1000;
  blinkOff = inputs[1]->floatValue() * 1000;
  nextBlinkTime = frugal_iot.powercontroller->sleepSafeMillis() + (outputs[0]->boolValue() ? blinkOn : blinkOff) ; // Blink after new blink time
}

void Control_Blinken::loop() {
  if (nextBlinkTime <= frugal_iot.powercontroller->sleepSafeMillis()) {
    bool next = !outputs[0]->boolValue();
    ((OUTbool*)outputs[0])->set(next); // Will send message
    nextBlinkTime = frugal_iot.powercontroller->sleepSafeMillis() + (next ? blinkOn : blinkOff);
  }
}
