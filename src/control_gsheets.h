#ifndef CONTROL_GSHEETS_H
#define CONTROL_GSHEETS_H

#include "_settings.h"
#ifdef CONTROL_GSHEETS_WANT

#include "system_base.h" // for IOtype
#include "control_logger.h"

class Control_Gsheets : public Control_Logger {
  public:
    String* url;  // May be const char* ... not sure yet
    Control_Gsheets(const char* name, String* url);
    Control_Gsheets(const char* name, const char* const url);
    void track(const char* col, String* topicPath);
    void track(const char* col, const char* topicPath);
    void sendGoogle(String* payload);
    void act();
};

#endif // CONTROL_GSHEETS_WANT
#endif // CONTROL_GSHEETS_H