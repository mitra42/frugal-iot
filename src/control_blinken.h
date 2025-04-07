#ifndef CONTROL_BLINKEN_H
#define CONTROL_BLINKEN_H

/*  Demo blinking led on board
 *
 * Configuration options
 * Optional: CONTROL_BLINKEN_DEBUG
*/

#include "control.h"

class ControlBlinken : public Control {
  public:
    unsigned long nextBlinkTime = 0;
    unsigned long blinkOn = 0; // in milliseconds (converted from seconds in act)
    unsigned long blinkOff = 0; // in milliseconds (converted from seconds in act)
    ControlBlinken(const char* const name, float secsOn, float secsOff);
    void act(); // Override in Control
    void loop(); // Override in FrugalBase
};

#endif // CONTROL_BLINKEN_H