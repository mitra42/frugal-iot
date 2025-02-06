#ifdef CONTROL_HYSTERISIS_WANT

#include "control.h"

/* Will define ControlHysteris class here, then replace instantiation in frugal_iot.ino */

class ControlHysterisis : public Control {
  public:
    ControlHysterisis(String name, float now, float min, float max);
};
#endif //CONTROL_HYSTERISIS_WANT
