/* Frugal IoT - Control_Hysteresis - Parameterised control with hysteresis built in.
 *
 */

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "misc.h"
#include "control_hysteresis.h"

#ifdef CONTROL_HYSTERESIS_DEBUG
void Control_Hysteresis::debug(const char* const where) {
  Serial.printf("%s: ",where);
  Serial.printf(" now=%f ", inputs[0]->floatValue());
  if (((INfloat*)inputs[0])->wiredPath) { Serial.print(((INfloat*)inputs[0])->wiredPath); }
  Serial.printf(" greater=%d ", inputs[1]->boolValue());
  if (((INfloat*)inputs[1])->wiredPath) { Serial.print(((INfloat*)inputs[1])->wiredPath); }
  Serial.printf(" limit=%f ", inputs[2]->floatValue());
  if (((INfloat*)inputs[2])->wiredPath) { Serial.print(((INfloat*)inputs[2])->wiredPath); }
  Serial.printf(" hysteresis=%f ", inputs[3]->floatValue());
  if (((INfloat*)inputs[3])->wiredPath) { Serial.print(((INfloat*)inputs[3])->wiredPath); }
  Serial.printf( " out=%d ", outputs[0]->boolValue());
  if (((INbool*)outputs[0])->wiredPath) { Serial.print(((INbool*)outputs[0])->wiredPath); }
  Serial.println();
}
#endif //CONTROL_HYSTERESIS_DEBUG

void Control_Hysteresis::act() {
  const float now = inputs[0]->floatValue();
  const bool greater = inputs[1]->boolValue();
  const float lim = inputs[2]->floatValue();
  const float hysteresis = inputs[3]->floatValue();
  #ifdef CONTROL_HYSTERESIS_DEBUG
    Serial.print("XXX now="); Serial.print(now); Serial.print(" greater ="); Serial.print(greater); Serial.print(" lim"); Serial.print(lim); Serial.print(" out="); Serial.println(((OUTbool*)outputs[0])->value);
  #endif
  if ((greater && (now > (lim + hysteresis))) || (!greater && (now < (lim - hysteresis)))) {
      ((OUTbool*)outputs[0])->set(true);
  }
  if ((greater && (now < (lim - hysteresis))) || (!greater &&  (now > (lim + hysteresis)))) {
    ((OUTbool*)outputs[0])->set(false);
  }
  #ifdef CONTROL_HYSTERESIS_DEBUG
    debug("Control_Hysteresis after act");
  #endif
  // If  lim-histerisis < hum < lim+histerisis then don't change setting (or reverse if !greater)
};

Control_Hysteresis::Control_Hysteresis (const char* const id, const char * const name, float now, uint8_t width, float min, float max) 
: Control(id, name,
  std::vector<IN*> {
    new INfloat(id, "now", "Now", now, width, min, max, DEFAULT_controlhysteresis_now_min, DEFAULT_controlhysteresis_now_max, DEFAULT_controlhysteresis_now_color, true),
    new INbool(id, "greater", "Greater than", true, DEFAULT_controlhysteresis_greater_color, false),
    new INfloat(id, "limit", "Limit", now, width, min, max, DEFAULT_controlhysteresis_limit_min, DEFAULT_controlhysteresis_limit_max, DEFAULT_controlhysteresis_limit_color, true),
    new INfloat(id, "hysteresis", "Hysteresis", 0, width, min, max, DEFAULT_controlhysteresis_hysteresis_min, DEFAULT_controlhysteresis_hysteresis_max, DEFAULT_controlhysteresis_hysteresis_color, false)
  },
  std::vector<OUT*> {
    // Note assumptions here, and in superclasses e.g. Control_Sonoff that output[0]="out" and is the output
    new OUTbool(id, "out", "Out", false, DEFAULT_controlhysteresis_out_color, true)
  }
) {
  #ifdef CONTROL_HYSTERESIS_DEBUG
    debug("Control_Hysteresis after instantiation");
  #endif
};


