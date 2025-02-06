/* 
 * Hysterisis driven control
 */

#include "_settings.h"  // Settings for what to include etc

#ifdef CONTROL_HYSTERISIS_WANT


#include <Arduino.h>
#include "control_hysterisis.h"

/* Will define ControlHysteris class here, then replace instantiation in frugal_iot.ino */

// TODO-25 move this function into the class 

Control::TCallback hysterisisAction = [](Control* self) {
    const float hum = self->inputs[0]->floatValue();
    const float lim = self->inputs[1]->floatValue();
    const float hysterisis = self->inputs[2]->floatValue();
    if (hum > (lim + hysterisis)) {
        self->outputs[0]->set(1);
    }
    if (hum < (lim - hysterisis)) {
        self->outputs[0]->set(0);
    }
  // If  lim-histerisis < hum < lim+histerisis then don't change setting
};

ControlHysterisis::ControlHysterisis (String name, float now, float min, float max) : Control(
  name + "_control",
  std::vector<IN*> {
    new INfloat(name + "_now", now, name + "_now", min, max, "black", true),
    new INfloat(name + "_limit", now, name + "_limit", min, max, "black", false),
    new INfloat("hysterisis", max/4, "hysterisis", 0, max/2, "black", false)
  },
  std::vector<OUT*> {
    new OUTbool(name + "_hysterisis_out", false, name + "_hysterisis_out", "black", true), 
  },
  std::vector<Control::TCallback> {
    hysterisisAction
  }) {};
#endif //CONTROL_HYSTERISIS_WANT

