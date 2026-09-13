/* See sensor_tank.h for the hardware, the calibration flags, and why "no sender" and "empty tank"
 * are deliberately different answers here when OSPIT conflates them.
 */

#include "sensor_tank.h"
#include "system/language.h" // for Texts
#include "Frugal-IoT.h"

// offset/scale map raw counts onto 0..100 %: convert() in Sensor_Analog is (v - offset) * scale,
// so offset is the empty reading and scale turns the span into percent.
Sensor_Tank::Sensor_Tank(const char* const id, const char* const name, uint8_t pin, bool retain,
                         int raw_empty, int raw_full, int raw_disconnected)
: Sensor_Analog(id, name, pin, 3,
                DEFAULT_tank_tank_min, DEFAULT_tank_tank_max,
                raw_empty,
                (raw_full == raw_empty) ? 0.0f : (100.0f / (float)(raw_full - raw_empty)),
                DEFAULT_tank_tank_color, retain),
  raw_disconnected(raw_disconnected)
{ }

/* True only for a reading a connected sender could actually have produced.
 *
 * Returning false publishes "nan" rather than a number, which is what lets Control_Irrigation
 * tell "there is no tank sensor on this node" from "the tank is empty" - see sensor_tank.h.
 */
bool Sensor_Tank::validate(int v) {
  return v < raw_disconnected;
}

// Clamp, because a sender slightly out of calibration should read 100% rather than 103% - and
// because Control_Irrigation compares against thresholds that are expressed as percentages.
float Sensor_Tank::convert(int v) {
  const float pc = Sensor_Analog::convert(v);
  if (pc < 0.0f) {
    return 0.0f;
  } else if (pc > 100.0f) {
    return 100.0f;
  } else {
    return pc;
  }
}

// Tare against an empty tank, then enter the true percentage with it full. Sensor_Analog::dispatch
// does the work and persists both offset and scale - see the note in sensor_tank.h.
void Sensor_Tank::captiveLines(AsyncResponseStream* response) {
  frugal_iot.captive->addButton(response, id, "output", "0", T->Tare);
  frugal_iot.captive->addNumber(response, id, "output", String(output->floatValue(), 3),
                                T->Calibrate, output->min, output->max);
}
