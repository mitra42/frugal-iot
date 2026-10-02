// Deep Sleep issues: none - redrawn on the next act().
/* Frugal IoT - Control OLED Single
 *
 * Shows one value, large, on the node's OLED - with an optional unit to its right and an optional
 * label below it:
 *
 *     1234mm
 *     Distance
 *
 * The value arrives on a wireable input, `control_oled_single/value`, so wire it to whatever
 * should be shown - a sensor on this node, or one on another node:
 *
 *   Control_Oled_Single* cos = new Control_Oled_Single("Control OLED", "mm", "Distance");
 *   frugal_iot.controls->add(cos);
 *   cos->value->wireTo(frugal_iot.messages->path("hcsr04/hcsr04"));
 *
 * Text sizes come from the display's visible height, so the same code suits both panels in common
 * use - the 128x64 on the LoRa boards and the 72x40 HW675 - and the 128x128 SSD1327. Compare
 * examples/remotedisplay/control_oled_ht, which it is modelled on.
 *
 * Only declared with ACTUATOR_OLED_WANT, so wrap its use in the sketch in #ifdef ACTUATOR_OLED_WANT.
 * One per node, as the module id is fixed.
 */
#ifndef CONTROL_OLED_SINGLE_H
#define CONTROL_OLED_SINGLE_H

#include "_settings.h"
#ifdef ACTUATOR_OLED_WANT // Using it on a board with no OLED is a compile error, not a link error

#include "control/oled.h"

class Control_Oled_Single : public Control_Oled {
  public:
    // unit and label are optional - nullptr to leave out; decimals is places after the point
    Control_Oled_Single(const char* name, const char* unit = nullptr, const char* label = nullptr, uint8_t decimals = 0);
    INfloat* value;
    void act() override;
  protected:
    const char* unit;
    const char* label;
};
#endif // ACTUATOR_OLED_WANT
#endif // CONTROL_OLED_SINGLE_H
