#ifndef MISC_H
#define MISC_H

// And some useful functions used in various places - ifdef them for places they are used

// Sprintf is similar to StringF but uses a different mechanism - it came with WiFiSettings.cpp
#define Sprintf(f, ...) ({ char* s; asprintf(&s, f, __VA_ARGS__); String r = s; free(s); r; })

const String StringF(const char* format, ...);
const String* newStringF(const char* format, ...);

const char* lprintf(size_t buffer_size, const char* format, ...);

void split(const String& str, String parts[], int& count); // This is currently only used in match_topic
bool match_topic(const String& topic, const String& pattern);

#endif //MISC_H