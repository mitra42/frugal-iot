#ifndef CONTROL_GSHEETS_H
#define CONTROL_GSHEETS_H

#include "_settings.h"
#ifdef CONTROL_GSHEETS_WANT

#include "_base.h" // for IOtype
#include "control.h"

class Control_Gsheets : public Control {
  public:
    std::vector<IN*> inputs; // Vector of inputs
    Control_Gsheets(const char* name);
    void track(IOtype t, const char* topicPath);
    void track(IOtype t, String* topicPath);
};


#endif // CONTROL_GSHEETS_WANT
#endif // CONTROL_GSHEETS_H