/* 
 * Hysterisis driven control
 */

#include "_settings.h"  // Settings for what to include etc

#ifdef CONTROL_HYSTERISIS_WANT


#include <Arduino.h>
#include "misc.h"
#include "control_hysterisis.h"

void ControlHysterisis::act() {
  const float hum = inputs[0]->floatValue();
  const float lim = inputs[1]->floatValue();
  const float hysterisis = inputs[2]->floatValue();
  #ifdef CONTROL_HYSTERISIS_DEBUG
    Serial.print(F("hum=")); Serial.print(hum); Serial.print(F(" lim=")); Serial.print(lim); Serial.print(F(" hysterisis=")); Serial.print(hysterisis);
  #endif
  if (hum > (lim + hysterisis)) {
      ((OUTbool*)outputs[0])->set(true);
  }
  if (hum < (lim - hysterisis)) {
    ((OUTbool*)outputs[0])->set(false);
  }
  // If  lim-histerisis < hum < lim+histerisis then don't change setting
};

ControlHysterisis::ControlHysterisis (const char* const id, const char * const name, float now, float min, float max) 
: Control(lprintf(strlen(name)+9, "%s_control", name),
  std::vector<IN*> {
    new INfloat(id, "now", "Now", now, min, max, "black", true),
    new INfloat(id, "limit", "Limit", now, min, max, "black", true),
    new INfloat(id, "hysterisis", "Hysterisis", 0, 0, max/2, "black", false),
  },
  std::vector<OUT*> {
    new OUTbool(id, "out", "Out", false, "black", true), 
  }
) {
  #ifdef CONTROL_HYSTERISIS_DEBUG
    debug("ControlHysterisis after instantiation");
  #endif
};

#endif //CONTROL_HYSTERISIS_WANT

