
// Some common functions - ifdef them for the functions that use them
#include <Arduino.h>
#include "_settings.h"
#include "system_frugal.h"

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
/* Removed as cause of memory leaks 
//TODO For now just copying above, could refactor
const String* newStringF(const char* format, ...) {
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
      return new String(buffer2); // Return a String from the buffer
    }
    va_end(args);
    return new String(buffer); // Note - string returned on stack so should be safe
}
*/
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

// Function to split a String into parts based on delimiter "/"
void split(const String& str, String parts[], int& count) {
    int start = 0;
    int index = 0;

    for (uint16_t i = 0; i <= str.length(); i++) {
        if (i == str.length() || str.charAt(i) == '/') {
            parts[index++] = str.substring(start, i);
            start = i + 1;
        }
    }
    count = index; // Set the number of parts split
}

// Function to match topic with pattern
bool match_topic(const String& topic, const String& pattern) {
    String topic_parts[10];  // Max 10 levels for simplicity - currently using 6 org/project/node/module/input/parameter
    String pattern_parts[10];  // Max 10 levels for simplicity
    int topic_size = 0, pattern_size = 0;

    // Split the topic and pattern into parts
    split(topic, topic_parts, topic_size);
    split(pattern, pattern_parts, pattern_size);

    int i = 0, j = 0;

    // Matching the topic with the pattern
    while (i < topic_size && j < pattern_size) {
        if (pattern_parts[j] == "+") {
            // + matches exactly one level, so move both indices
            i++;
            j++;
        }
        else if (pattern_parts[j] == "#") {
            // # matches any number of levels, so return true
            return true;
        }
        else if (topic_parts[i] == pattern_parts[j]) {
            // Exact match for a topic part
            i++;
            j++;
        }
        else {
            // No match, return false
            return false;
        }
    }

    // If pattern ends with #, it matches the rest of the topic
    if (j < pattern_size && pattern_parts[j] == "#") {
        return true;
    }

    // Check if both topic and pattern are fully matched
    return i == topic_size && j == pattern_size;
}

// TODO-MEM comment out calls to this when not needed
void heap_print(const __FlashStringHelper *msg) {
  #ifdef SYSTEM_MEMORY_DEBUG
    #ifdef ESP8266
      if (msg) { Serial.print(msg); }  Serial.print(F(" heap=")); Serial.println(system_get_free_heap_size());  // https://www.esp8266.com/viewtopic.php?p=82839
    #elif defined(ESP32) //TODO-128 should be able to find equivalent on ESP8266
      if (msg) { Serial.print(msg); } ; Serial.print(F(" heap=")); Serial.print(esp_get_free_heap_size()); 
      Serial.print(F(" min heap=")); Serial.println(esp_get_minimum_free_heap_size());
    #endif //ESP32
  #endif //SYSTEM_MEMORY_DEBUG
}
