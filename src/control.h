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
#include "system_base.h"

class Control : public System_Base {
  public:
    std::vector<IN*> inputs; // Vector of inputs
    std::vector<OUT*> outputs; // Vector of outputs

    Control(const char * const id, const char * const name, std::vector<IN*> i, std::vector<OUT*> o); 
    void setup();
    virtual void act();
    String advertisement();
    void dispatchTwig(const String &topicControlId, const String &topicLeaf, const String &payload, bool isSet);
    virtual void dispatchPath(const String &topicPath, const String &payload);
    #ifdef CONTROL_DEBUG
      virtual void debug(const char* const blah);
    #endif //CONTROL_DEBUG
};
    
#endif //CONTROL_H
