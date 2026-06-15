#ifndef CONTROL_LCD_SHT_H
#define CONTROL_LCD_SHT_H

#include "_settings.h"
#ifdef ACTUATOR_LCD_WANT

#include "control.h"

class Control_LCD_SHT : public Control {
  public:
    INfloat* temperature;
    INfloat* humidity;
    OUTtext* message;
    Control_LCD_SHT();
    void act() override;
};

#endif // ACTUATOR_LCD_WANT
#endif // CONTROL_LCD_SHT_H
