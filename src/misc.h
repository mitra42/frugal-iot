#ifndef MISC_H
#define MISC_H

// And some useful functions used in various places - ifdef them for places they are used

// Sprintf is similar to StringF but uses a different mechanism - it came with WiFiSettings.cpp
#define Sprintf(f, ...) ({ char* s; asprintf(&s, f, __VA_ARGS__); String r = s; free(s); r; })

const String StringF(const char* format, ...);
//const String* newStringF(const char* format, ...);

const uint8_t* lprintf(size_t buffer_size, const char* format, ...);

void split(const String& str, String parts[], int& count); // This is currently only used in match_topic
bool match_topic(const String& topic, const String& pattern);

void heap_print(const __FlashStringHelper *msg);

void shouldBeDefined();

/* The on-the-wire form of "this sensor currently has no reading".
 *
 * A sensor whose validate() fails publishes this instead of publishing nothing, so that the
 * absence of a reading is state that propagates to controls and to the UX, rather than being
 * indistinguishable from "the value has not changed". See CLAUDE.md "Invalid readings".
 *
 * Canonical and lower case, because String(NAN, width) does NOT reliably produce it: Arduino's
 * String(double, dp) calls dtostrf(v, dp+2, dp, buf), and dtostrf right-justifies to that
 * minimum field width - so a width of 2 yields " nan" with a leading space while a width of 1
 * yields "nan". OUTfloat::StringValue() therefore emits this constant directly rather than
 * letting the formatter decide.
 */
#define IO_PAYLOAD_INVALID "nan"

/* Value comparison for the send-on-change tests, NaN-aware.
 *
 * IEEE says NaN != NaN, so a plain `newvalue != value` reports "changed" on every read while a
 * sensor is invalid - which would republish IO_PAYLOAD_INVALID every cycle and re-run every
 * wired control with it. changed(NAN, NAN) is false; changed(NAN, 5) and changed(5, NAN) are true.
 */
bool changed(float a, float b);
// Declared explicitly so a double cannot bind to the template below and silently lose the NaN
// handling - the template would compare with == and report NAN as perpetually changed.
bool changed(double a, double b);
// Types with no NaN - uint16_t, bool, String - just compare.
template<typename T> bool changed(const T& a, const T& b) { return !(a == b); }

#endif //MISC_H