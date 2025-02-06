
// Some common functions - ifdef them for the functions that use them
#include "_settings.h"

#if defined(SYSTEM_TIME_WANT) || defined(CONTROL_WANT)
#include <Arduino.h> // For String
//#include <stdio.h> // Doesnt appear to be needed - was in sample code from Jonathan Semple
//#include <stdarg.h> // Doesnt appear to be needed - was in sample code from Jonathan Semple
//#include "WString.h" // Doesnt appear to be needed - was in sample code from Jonathan Semple

const String StringF(const char* format, ...) {
    char buffer[200];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    return String(buffer);
}
#endif 

#if defined(CONTROL_WANT) || defined(CONTROL_HYSTERISIS_WANT)
  const char* lprintf(size_t buffer_size, const char* format, ...) {
    char* buffer = new char[buffer_size];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, buffer_size, format, args);
    return buffer; // This buffer should stay in scope - and must be explicitly freed up by delete() if not wanted.
  }
#endif 