#ifndef CONTROL_BLINKEN_H
#define CONTROL_BLINKEN_H

/*  Demo blinking led on board
 *
 * Configuration options
 * Optional: CONTROL_BLINKEN_S CONTROL_BLINKEN_DEBUG
*/

#include "control.h"

// #define CONTROL_BLINKEN_WANT // Define in _local.h if want to test with this
#define CONTROL_BLINKEN_ADVERTISEMENT "\n  -\n    topic: control_blinken_seconds\n    name: Blink period (s)\n    type: int\n    min: 1\n    max: 60\n    display: slider\n    rw: w"

class ControlBlinken : public Control {
  public:
    unsigned long nextBlinkTime = 0;
    unsigned long blinkSpeed = 0; // in milliseconds (converted from seconds in constructor)
    ControlBlinken(const char* const name, float blinkSpeed);
    void act(); // Override in Control
    void loop(); // Override in FrugalBase
};

#endif // CONTROL_BLINKEN_H