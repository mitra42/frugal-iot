#ifndef ACTUATOR_LEDBUILTIN_H
#define ACTUATOR_LEDBUILTIN_H

/* Configuration options
 * Optional: ACTUATOR_LEDBUILTIN_DEBUG ACTUATOR_LEDBUILTIN_PIN - defaults
*/


namespace aLedbuiltin {
void setup();
void loop();
extern bool value;
} // namespace aLedbuiltin
#endif // ACTUATOR_LEDBUILTIN_H