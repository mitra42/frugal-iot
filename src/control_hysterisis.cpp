/* 
 * Hysterisis driven control
 */

#include "_settings.h"  // Settings for what to include etc

#ifdef CONTROL_HYSTERISIS_WANT


#include <Arduino.h>
#include "misc.h"
#include "control_hysterisis.h"

#ifdef CONTROL_HYSTERISIS_DEBUG
void ControlHysterisis::debug(const char* const where) {
  Serial.printf("%s: ",where);
  Serial.printf(" now=%f ", inputs[0]->floatValue());
  if (((INfloat*)inputs[0])->wiredPath) { Serial.print(*(((INfloat*)inputs[0])->wiredPath)); }
  Serial.printf(" greater=%d ", inputs[1]->boolValue());
  if (((INbool*)inputs[1])->wiredPath) { Serial.print(*(((INfloat*)inputs[1])->wiredPath)); }
  Serial.printf(" limit=%f ", inputs[2]->floatValue());
  if (((INfloat*)inputs[2])->wiredPath) { Serial.print(*(((INfloat*)inputs[2])->wiredPath)); }
  Serial.printf(" hysterisis=%f ", inputs[3]->floatValue());
  if (((INfloat*)inputs[3])->wiredPath) { Serial.print(*(((INfloat*)inputs[3])->wiredPath)); }
  Serial.printf( " out=%d ", outputs[0]->boolValue());
  if (((INfloat*)outputs[0])->wiredPath) { Serial.print(*(((INfloat*)outputs[0])->wiredPath)); }
  Serial.println();
}
#endif //CONTROL_HYSTERISIS_DEBUG

void ControlHysterisis::act() {
  const float now = inputs[0]->floatValue();
  const float greater = inputs[1]->boolValue();
  const float lim = inputs[2]->floatValue();
  const float hysterisis = inputs[3]->floatValue();
  if ((greater && (now > (lim + hysterisis))) || (!greater && (now < (lim - hysterisis)))) {
      ((OUTbool*)outputs[0])->set(true);
  }
  if ((greater && (now < (lim - hysterisis))) || (!greater &&  (now > (lim + hysterisis)))) {
    ((OUTbool*)outputs[0])->set(false);
  }
  #ifdef CONTROL_HYSTERISIS_DEBUG
    debug("ControlHysterisis after act");
  #endif
  // If  lim-histerisis < hum < lim+histerisis then don't change setting (or reverse if !greater)
};

ControlHysterisis::ControlHysterisis (const char* const id, const char * const name, float now, uint8_t width, float min, float max) 
: Control(id, name,
  std::vector<IN*> {
    new INfloat(id, "now", "Now", now, width, min, max, "black", true),
    new INbool(id, "greater", "Greater than", true, "black", false),
    new INfloat(id, "limit", "Limit", now, width, min, max, "black", true),
    new INfloat(id, "hysterisis", "Hysterisis", 0, width, 0, max/2, "black", false)
  },
  std::vector<OUT*> {
    new OUTbool(id, "out", "Out", false, "black", true)
  }
) {
  #ifdef CONTROL_HYSTERISIS_DEBUG
    debug("ControlHysterisis after instantiation");
  #endif
};

#endif //CONTROL_HYSTERISIS_WANT

