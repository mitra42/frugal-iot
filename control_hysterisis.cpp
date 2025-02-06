/* 
 * Hysterisis driven control
 */

#include "_settings.h"  // Settings for what to include etc

#ifdef CONTROL_HYSTERISIS_WANT


#include <Arduino.h>
#include "misc.h"
#include "control_hysterisis.h"

/* Will define ControlHysteris class here, then replace instantiation in frugal_iot.ino */

// TODO-25 move this function into the class 

void ControlHysterisis::act() {
    const float hum = inputs[0]->floatValue();
    const float lim = inputs[1]->floatValue();
    const float hysterisis = inputs[2]->floatValue();
    if (hum > (lim + hysterisis)) {
        outputs[0]->set(1);
    }
    if (hum < (lim - hysterisis)) {
        outputs[0]->set(0);
    }
  // If  lim-histerisis < hum < lim+histerisis then don't change setting
};

ControlHysterisis::ControlHysterisis (const char* const name, float now, float min, float max) : Control(
  lprintf(strlen(name)+8, "%s_control", name),
  std::vector<IN*> {
    new INfloat(lprintf(strlen(name)+4, "%s Now", name), now, lprintf(strlen(name)+4, "%s_now", name), min, max, "black", true),
    new INfloat(lprintf(strlen(name)+6, "%s Limit", name), now, lprintf(strlen(name)+6, "%s_limit", name), min, max, "black", false),
    new INfloat("Hysterisis", max/4, lprintf(strlen(name)+11, "%s_hysterisis", name), 0, max/2, "black", false)
  },
  std::vector<OUT*> {
    new OUTbool(lprintf(strlen(name)+4, "%s Out", name), false, lprintf(strlen(name)+15, "%s_hysterisis_out", name), "black", true), 
  },
  std::vector<Control::TCallback> {} // No action - its subclassed
) {};

#endif //CONTROL_HYSTERISIS_WANT

