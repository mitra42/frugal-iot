#ifdef CONTROL_HYSTERISIS_WANT

#include "control.h"

/* Will define ControlHysteris class here, then replace instantiation in frugal_iot.ino */

class ControlHysterisis : public Control {
  public:
    ControlHysterisis(const char* const id, const char * const name, float now, uint8_t width, float min, float max);
    void act();
    #ifdef  CONTROL_HYSTERISIS_DEBUG
      void debug(const char* const where);
    #endif //CONTROL_HYSTERISIS_DEBUG
};
#endif //CONTROL_HYSTERISIS_WANT
