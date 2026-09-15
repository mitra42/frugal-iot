// Deep Sleep issues: none in itself - inputs are restored from LittleFS; see the individual controls.
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
#include "system/base.h"
#include "system/io.h"
#include "system/message.h"

class Control : public System_Base {
  public:
    std::vector<IN*> inputs; // Vector of inputs
    std::vector<OUT*> outputs; // Vector of outputs
    bool enabled = true; // Set false to suppress act() drawing/output

    Control(const char * const id, const char * const name, std::vector<IN*> i, std::vector<OUT*> o);
    void setup() override;
    /* False if any input is currently carrying "no reading" - see IN::isValid().
     *
     * A control that acts on physical hardware should generally test this in act() before
     * trusting floatValue(), which deliberately keeps returning the last good value. Types with
     * no NaN always report valid, so this is safe on a control with mixed input types.
     *
     * What to do when it is false is the control's decision, not this class's: holding the
     * output is right for a thermostat but wrong for an irrigation valve, which wants to close.
     */
    bool allInputsValid();
    virtual void act();
    void discover() override;
    void dispatch(System_Message &msg) override;
    #ifdef CONTROL_DEBUG
      virtual void debug(const char* const blah);
    #endif //CONTROL_DEBUG
};
    
#endif //CONTROL_H
