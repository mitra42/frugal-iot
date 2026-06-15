#ifndef CONTROL_OLED_LORAMESHER_H
#define CONTROL_OLED_LORAMESHER_H

#include "control_oled.h"

// Defaults here, since not in schema yet - and may never be
#define DEFAULT_control_oled_loramesher_battery_max 5000
#define DEFAULT_control_oled_loramesher_battery_min 0

class Control_Oled_LoRaMesher : public Control_Oled {
  public:
    //INfloat* temperature;
    //INfloat* humidity;
    INfloat* battery;
    Control_Oled_LoRaMesher(const char* name);
    void act() override;
};
#endif // CONTROL_OLED_LORAMESHER_H