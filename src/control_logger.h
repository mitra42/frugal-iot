#ifndef CONTROL_LOGGER_H
#define CONTROL_LOGGER_H

#include "_settings.h"

#include "control.h"

class Control_Logger : public Control {
  public:
    Control_Logger(const char* id, const char* name);
    Control_Logger(const char* id, const char* name, std::vector<IN*> i);
    void setup(); 
  };  
#endif // CONTROL_LOGGER_H