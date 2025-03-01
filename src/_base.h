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
    virtual void dispatch(const String &topic_msg, const String &payload);
    static void dispatchAll(const String &topic, const String &payload); //TODO could be topicLeaf or topicPath - check and clarify
}; // Class FrugalBase

#endif // BASE_H