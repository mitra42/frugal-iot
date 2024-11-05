#ifndef ACTUATOR_RELAY_H
#define ACTUATOR_RELAY_H

/* Configuration options
 * Optional: ACTUATOR_LEDBUILTIN_DEBUG ACTUATOR_LEDBUILTIN_PIN - defaults
*/

#define ACTUATOR_RELAY_TOPIC "relay"

namespace aRelay {
void setup();
void set(int value);
void loop();
extern bool value;
extern String *topic;
} // namespace aRelay
#endif // ACTUATOR_RELAY_H