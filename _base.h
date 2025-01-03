#ifndef BASE_H
#define BASE_H

#include <Arduino.h>

class Frugal_Base {
  public:
    Frugal_Base* next;

    Frugal_Base();
    virtual void setup();
    static void setupAll();
    virtual void loop();
    static void loopAll();
    virtual void dispatch(String &topic, String &payload);
    static void dispatchAll(String &topic, String &payload);
}; // Class FrugalBase

#endif // BASE_H