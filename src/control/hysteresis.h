#ifndef CONTROL_HYSTERESIS_H
#define CONTROL_HYSTERESIS_H

#include "control/control.h"

/* Will define ControlHysteris class here, then replace instantiation in frugal_iot.ino */

class Control_Hysteresis : public Control {
  public:
    Control_Hysteresis(const char* const id, const char * const name, float now, uint8_t width, float min, float max);
    void act() override;
  protected:
    void actInner(); // The threshold logic, run only when allInputsValid()
  public:
    #ifdef  CONTROL_HYSTERESIS_DEBUG
      void debug(const char* const where);
    #endif //CONTROL_HYSTERESIS_DEBUG
};
#endif // CONTROL_HYSTERESIS_H
