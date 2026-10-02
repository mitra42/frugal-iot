/*
 * Ultrasonic distance sensor over RS485 / Modbus RTU (e.g. A01ANY4B).
 *
 * NOTE - AS OF 2026-09-19 THIS IS UNTESTED CODE AS WE DONT HAVE THE HARDWARE -
 * BUG REPORTS OR CONFIRMATION THAT IT WORKS VERY WELCOME
 *
 * See sensor/ultrasonicmodbus.h for build flags, sensor/ultrasonic.h for the offset/scale
 * convention, and system/modbus.h for the bus itself.
 */

#include "sensor/ultrasonicmodbus.h" // Includes _settings.h, and is a no-op unless the slave id is defined

#ifdef SENSOR_ULTRASONIC_SLAVE_ID

#include <Arduino.h>

Sensor_UltrasonicModbus::Sensor_UltrasonicModbus(const char* const id, const char * const name, float max, const char* color, const bool retain,
  System_RS485* bus, float offset, float scale, uint8_t slave_id, uint16_t reg)
  : Sensor_Ultrasonic(id, name, max, color, retain, offset, scale),
    modbus(slave_id, bus),
    reg(reg)
  { }

void Sensor_UltrasonicModbus::setup() {
  Sensor_Ultrasonic::setup(); // Will readConfigFromFS and set offset and scale, do before the bus
  modbus.initialize();
}

// Raw distance in mm, or NAN if the slave did not answer. Sensor_Float::validate rejects NAN,
// which publishes "nan" rather than silently leaving the last reading standing - see
// "Invalid readings" in CLAUDE.md.
float Sensor_UltrasonicModbus::readFloat() {
  uint16_t raw = 0;
  float v = NAN;
  if (modbus.readRegister(reg, &raw)) {
    v = raw;
  }
  #ifdef SENSOR_ULTRASONIC_DEBUG
    Serial.print(F("Ultrasonic: connected=")); Serial.print(modbus.connected);
    Serial.print(F(" raw=")); Serial.println(v);
  #endif
  return v;
}

#endif // SENSOR_ULTRASONIC_SLAVE_ID
