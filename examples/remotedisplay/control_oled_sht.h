#ifndef CONTROL_OLED_SHT_H
#define CONTROL_OLED_SHT_H

#include "control_oled.h"

class Control_Oled_SHT : public Control_Oled {
  public:
    INfloat* temperature;
    INfloat* humidity;
    INfloat* battery;
    Control_Oled_SHT(const char* name);
    void act() override;
};
#endif // CONTROL_OLED_SHT_H