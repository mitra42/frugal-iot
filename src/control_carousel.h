#ifndef CONTROL_CAROUSEL_H
#define CONTROL_CAROUSEL_H

#include "control.h"
#include <vector>

class Control_Carousel : public Control {
  public:
    std::vector<Control*> controls;
    uint8_t selected;
    INuint16* select;

    Control_Carousel(const char* name);
    void setup() override;
    void act() override;
};

#endif // CONTROL_CAROUSEL_H
