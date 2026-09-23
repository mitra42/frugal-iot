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

/* Drive, or release, a pair of "power this thing" pins. Either may be PIN_NONE.
 *
 * The 3v3 pin sources the supply and is driven HIGH to turn the thing on; the 0v pin sinks the
 * return and is driven LOW. Powering down puts BOTH back to high-impedance INPUT rather than
 * driving them to the off level, so nothing is back-fed through a chip's protection diodes while
 * its supply is gone.
 *
 * Free functions rather than methods because both halves of the library need them and they have
 * no state: System_Base::powerUp()/powerDown() for a sensor's or actuator's own pins, and
 * System_Interface for the pins that feed a whole bus (system/interface.h).
 *
 * pinMode(OUTPUT) is set on every power up, not once when the pins are declared, because
 * powering down leaves them as INPUTs - and digitalWrite() on an INPUT pin only switches the
 * internal pull-up, which is a few tens of microamps and nowhere near enough to run a sensor.
 *
 * Both return true if either pin was a real GPIO, i.e. if anything actually happened - which is
 * what tells a caller whether it needs to wait SYSTEM_POWER_ON_DELAY for the rail to come up.
 */
bool pinsPowerUp(uint8_t pin3v3, uint8_t pin0v);
bool pinsPowerDown(uint8_t pin3v3, uint8_t pin0v);

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