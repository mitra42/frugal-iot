#ifndef CONTROL_OLED_LORAMESHER_H
#define CONTROL_OLED_LORAMESHER_H

#include "control_oled.h"

class Control_Oled_LoRaMesher : public Control_Oled {
  public:
    //INfloat* temperature;
    //INfloat* humidity;
    INfloat* battery;
    Control_Oled_LoRaMesher(const char* name);
    void act() override;
};
#endif // CONTROL_OLED_LORAMESHER_H