#ifndef CONTROL_OLED_H
#define CONTROL_OLED_H

#include "_settings.h"

#include "control.h"

class Control_Oled : public Control {
  public:
    Control_Oled(const char* id, const char* name);
    Control_Oled(const char* id, const char* name, std::vector<IN*> ii);
    uint16_t color565(const char* p1); //TODO-149 maybe belongs in system_oled
    //void setup() override; 
  };  

class Control_Oled_SHT : public Control_Oled {
  public:
    INfloat* temperature;
    INfloat* humidity;
    Control_Oled_SHT(const char* name);
    void act() override;
};

#endif // CONTROL_OLED_H