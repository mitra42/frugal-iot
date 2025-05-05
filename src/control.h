/*
 * Generic base class for controls
 *
 * It makes some assumptions - e.g. max 3 float inputs, which if wrong may require refactoring. 
 *
 */

#ifndef CONTROL_H
#define CONTROL_H

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <vector>
#include "_base.h"

#if defined(CONTROL_HYSTERISIS_DEBUG) || defined(CONTROL_MPQ_DEBUG)
  #define CONTROL_DEBUG
#endif

class Control : public Frugal_Base {
  public:
    const char * const id; // ID of control
    const char * const name; // User friendly name of control
    std::vector<IN*> inputs; // Vector of inputs
    std::vector<OUT*> outputs; // Vector of outputs

    Control(const char * const id, const char * const name, std::vector<IN*> i, std::vector<OUT*> o); 
    void setup();
    void dispatch(const String &topicPath, const String &payload);
    virtual void act();
    String advertisement();
    static void setupAll();
    static void loopAll();
    static void dispatchPathAll(const String &topicPath, const String &payload);
    static String advertisementAll();
    #ifdef CONTROL_DEBUG
      void debug(const char* const blah);
    #endif //CONTROL_DEBUG
};

extern std::vector<Control*> controls;


#endif //CONTROL_H
