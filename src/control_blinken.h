/* Frugal IoT - Blinken demo, a simple control that blinks a light
 *
 * Optional: CONTROL_BLINKEN_DEBUG
 */

 #ifndef CONTROL_BLINKEN_H
#define CONTROL_BLINKEN_H

#include "control.h"

class Control_Blinken : public Control {
  public:
    unsigned long blinkOn = 0; // in milliseconds (converted from seconds in act)
    unsigned long blinkOff = 0; // in milliseconds (converted from seconds in act)
    Control_Blinken(const char* const id, const char* const name, float secsOn, float secsOff);
    void act() override; // Override in Control
    void loop() override; // Override in FrugalBase
  private:
    uint8_t timer_index; // Index into RTC timer array 
    void timer_set(unsigned long t);
};

#endif // CONTROL_BLINKEN_H
