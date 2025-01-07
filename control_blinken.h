#ifndef CONTROL_BLINKEN_H
#define CONTROL_BLINKEN_H

/* Configuration options
 * Required: CONTROL_BLINKEN_MS
*/


namespace cBlinken {
void setup();
void loop();
void dispatch(const String &topic, const String &payload);
} // namespace aBlinken
#endif // CONTROL_BLINKEN_H