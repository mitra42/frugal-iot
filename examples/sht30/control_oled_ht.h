#ifndef CONTROL_OLED_HT_H
#define CONTROL_OLED_HT_H

#include "control/oled.h"

class Control_Oled_HT : public Control_Oled {
  public:
    INfloat* temperature;
    INfloat* humidity;
    INfloat* battery;
    Control_Oled_HT(const char* name);
    void act() override;
};
#endif // CONTROL_OLED_HT_H