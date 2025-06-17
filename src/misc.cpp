
// Some common functions - ifdef them for the functions that use them
#include <Arduino.h>
#include "_settings.h"
#include "frugal_iot.h"

#include <Arduino.h> // For String
//#include <stdio.h> // Doesnt appear to be needed - was in sample code from Jonathan Semple
//#include <stdarg.h> // Doesnt appear to be needed - was in sample code from Jonathan Semple
//#include "WString.h" // Doesnt appear to be needed - was in sample code from Jonathan Semple

#define PRINTF_BUFFER_SIZE 256 // Size of buffer used by printf functions

const String StringF(const char* format, ...) {
    char buffer[PRINTF_BUFFER_SIZE]; // out of scope at end of this
    va_list args;
    va_start(args, format);
    uint16_t len = vsnprintf(buffer, PRINTF_BUFFER_SIZE, format, args);
    if (len >= PRINTF_BUFFER_SIZE) {
      #ifdef STRINGS_DEBUG
        Serial.print(F("StringF: vsnprintf buffer too small expanding to, len=")); Serial.print(len); Serial.print(F(" ")); Serial.println(buffer);
      #endif //STRINGS_DEBUG
      char buffer2[len + 1];
      len = vsnprintf(buffer2, len + 1, format, args);
      va_end(args);
      return String(buffer2); // Return a String from the buffer
    }
    va_end(args);
    return String(buffer); // Note - string returned on stack so should be safe
}

// Typical usage.   lprintf(strlen(a)+strlen(b)+2, "%s %s", a, b) note how add 1 for length
const char* lprintf(size_t buffer_size, const char* format, ...) {
  // Be careful with this, there is no compile time checking that the number of args matches the format 
  // and a mismatch will generate an Exception
  char* buffer = new char[buffer_size];
  va_list args;
  va_start(args, format);
  uint16_t len = vsnprintf(buffer, buffer_size, format, args);
  if (len >= buffer_size) {
    Serial.print(F("lprintf: buffer too small - need ")); Serial.print(F(" ")); Serial.println(buffer);
  }
  va_end(args);
  return buffer; // This buffer should stay in scope - and must be explicitly freed up by delete() if not wanted.
}

