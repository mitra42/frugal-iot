#ifndef CONTROL_HYSTERISIS_H
#define CONTROL_HYSTERISIS_H

#include "control.h"

/* Will define ControlHysteris class here, then replace instantiation in frugal_iot.ino */

class Control_Hysterisis : public Control {
  public:
    Control_Hysterisis(const char* const id, const char * const name, float now, uint8_t width, float min, float max);
    void act() override;
    #ifdef  CONTROL_HYSTERISIS_DEBUG
      void debug(const char* const where);
    #endif //CONTROL_HYSTERISIS_DEBUG
};
#endif // CONTROL_HYSTERISIS_H
