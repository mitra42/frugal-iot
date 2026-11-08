// Deep Sleep issues: none - the output state is persisted and restored, so it does not flip on waking.
#ifndef CONTROL_HYSTERESIS_H
#define CONTROL_HYSTERESIS_H

#include "control/control.h"

/* Will define ControlHysteris class here, then replace instantiation in frugal_iot.ino */

class Control_Hysteresis : public Control {
  public:
    /* `hysteresis` is the dead band either side of `limit` - the output only changes once the
     * input is past limit +/- hysteresis, so a value hovering at the threshold does not chatter.
     * Defaulted, so existing three-argument callers are unaffected; supply it where the real
     * thresholds are asymmetric, e.g. a battery cut-out at 11.9V that restores at 12.3V is
     * limit=12100, hysteresis=200 (in millivolts, which is what Sensor_Battery reports).
     */
    Control_Hysteresis(const char* const id, const char * const name, float now, uint8_t width, float min, float max, float hysteresis = 0);
    void act() override;
    // See System_Base. The other inputs are settings, so "now" is the one to wire; "out" is the
    // result - and stays the default in subclasses that add outputs, e.g. examples/sonoff
    IN* defaultInput() override { return inputs[0]; }    // now
    OUT* defaultOutput() override { return outputs[0]; } // out
  protected:
    void actInner(); // The threshold logic, run only when allInputsValid()
  public:
    #ifdef  CONTROL_HYSTERESIS_DEBUG
      void debug(const char* const where);
    #endif //CONTROL_HYSTERESIS_DEBUG
};
#endif // CONTROL_HYSTERESIS_H
