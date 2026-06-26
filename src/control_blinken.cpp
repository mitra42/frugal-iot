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
    new INfloat(id, "timeon", "Time On (s)", secsOn, 3, DEFAULT_controlblinken_timeon_min, DEFAULT_controlblinken_timeon_max, DEFAULT_controlblinken_timeon_color, true),
    new INfloat(id, "timeoff", "Time Off (s)", secsOff, 3, DEFAULT_controlblinken_timeoff_min, DEFAULT_controlblinken_timeoff_max, DEFAULT_controlblinken_timeoff_color, true),
  },
  std::vector<OUT*> {
    new OUTbool(id, "out", name, false, DEFAULT_controlblinken_out_color, true),
  }
), blinkOn(secsOn * 1000), blinkOff(secsOff * 1000),
   blink_next_ms(0)
   //timer_index(frugal_iot.powercontroller->timer_next())  // Alternative if want sleep safe timer
{
  #ifdef CONTROL_BLINKEN_DEBUG
    debug("Control_Blinken after instantiation");
    Serial.print(F("Control_Blinken: blinkOn: ")); Serial.print(blinkOn); Serial.print(F(" blinkOff: ")); Serial.println(blinkOff);
  #endif
};

void Control_Blinken::timer_set(unsigned long secs) {
  blink_next_ms = millis() + secs*1000;
  //frugal_iot.powercontroller->timer_set(timer_index, secs); // Alternative if want sleep safe timer
}
bool Control_Blinken::timer_expired() {
 return millis() >= blink_next_ms;  // Non sleep safe
 // return frugal_iot.powercontroller->timer_expired(timer_index); // Sleep safe 
}

void Control_Blinken::act() {
  // If calling act, then we know blinkSpeed changed
  blinkOn = inputs[0]->floatValue();
  blinkOff = inputs[1]->floatValue();
  timer_set( outputs[0]->boolValue() ? blinkOn : blinkOff);
}

void Control_Blinken::loop() {
  if (timer_expired()) {
    bool next = !outputs[0]->boolValue();
    ((OUTbool*)outputs[0])->set(next); // Will send message
    timer_set(next ? blinkOn : blinkOff);
  }
}
