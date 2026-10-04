// Deep Sleep issues: mesh status reads as unknown until the mesh has re-joined, which takes longer than a short wake.
#ifndef CONTROL_OLED_LORAMESHER_H
#define CONTROL_OLED_LORAMESHER_H

#include "control/oled.h"
// DEFAULT_control_oled_loramesher_* come from defaults.h, generated from the server's modules.yaml

class Control_Oled_LoRaMesher : public Control_Oled {
  public:
    //INfloat* temperature;
    //INfloat* humidity;
    INfloat* battery;
    Control_Oled_LoRaMesher(const char* name);
    void act() override;
};
#endif // CONTROL_OLED_LORAMESHER_H