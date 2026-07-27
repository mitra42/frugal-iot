#ifndef CONTROL_LCD_HT_H
#define CONTROL_LCD_HT_H

#include "_settings.h"
#ifdef ACTUATOR_LCD_WANT

#include "control/control.h"

class Control_LCD_HT : public Control {
  public:
    INfloat* temperature;
    INfloat* humidity;
    OUTtext* message;
    Control_LCD_HT();
    void act() override;
};

#endif // ACTUATOR_LCD_WANT
#endif // CONTROL_LCD_HT_H
