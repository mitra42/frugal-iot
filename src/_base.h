#ifndef BASE_H
#define BASE_H

#include <Arduino.h>

class Frugal_Base {
  public:
    Frugal_Base();
    virtual void setup();
    static void setupAll();
    virtual void loop();
    static void loopAll();
}; // Class FrugalBase

#endif // BASE_H