/* Sensor_Voltage - see voltage.h, especially the note on linearity */

#include "_settings.h"
#include <Arduino.h>
#include "sensor/voltage.h"

Sensor_Voltage::Sensor_Voltage(const char* const id, const char* const name, const uint8_t pin_init,
                               float voltage_divider, float min, float max,
                               int offset, const char* color, bool retain)
//(id, name, pin, width, min, max, offset, scale, color, retain)
: Sensor_Analog(id, name, pin_init, 0, min, max, offset, voltage_divider, color, retain)
{
  pinMode(pin, INPUT); // Before setup(), because Sensor_Battery may be read by checkLevel() first
}

#ifdef ESP32
int Sensor_Voltage::readInt() {
  powerUp(); // No-op unless the subclass called powerPins() - some boards gate the divider
  return analogReadMilliVolts(pin); // Factory-calibrated, unlike Sensor_Analog's analogRead()
}
#endif
