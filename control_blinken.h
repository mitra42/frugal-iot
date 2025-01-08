#ifndef CONTROL_BLINKEN_H
#define CONTROL_BLINKEN_H

/*  Demo blinking led on board
 *
 * Configuration options
 * Optional: CONTROL_BLINKEN_S CONTROL_BLINKEN_DEBUG
*/

// #define CONTROL_BLINKEN_WANT // Define in _local.h if want to test with this
#define CONTROL_BLINKEN_DEBUG
#define CONTROL_BLINKEN_ADVERTISEMENT "\n  -\n    topic: control_blinken_seconds\n    name: Blink period (s)\n    type: int\n    min: 1\n    max: 60\n    display: slider\n    rw: rw"


namespace cBlinken {
void setup();
void loop();
void dispatchLeaf(const String &topicleaf, const String &payload);
} // namespace aBlinken
#endif // CONTROL_BLINKEN_H