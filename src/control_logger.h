#ifndef CONTROL_LOGGER_H
#define CONTROL_LOGGER_H

#include "_settings.h"
#ifdef CONTROL_LOGGER_WANT

#include "control.h"

class Control_Logger : public Control {
  public:
    Control_Logger(const char* id, const char* name);
    Control_Logger(const char* id, const char* name, std::vector<IN*> i);
};  
#endif // CONTROL_LOGGER_WANT
#endif // CONTROL_LOGGER_H