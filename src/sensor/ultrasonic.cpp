/*
 * Ultrasonic distance sensor over RS485 / Modbus RTU (e.g. A01ANY4B).
 *
 * NOTE - AS OF 2026-08-03 THIS IS UNTESTED CODE
 *
 * See sensor/ultrasonic.h for build flags and the offset/scale convention,
 * and system/modbus.h for the bus itself.
 */

#include "sensor/ultrasonic.h" // Includes _settings.h, and is a no-op unless the slave id is defined

#ifdef SENSOR_ULTRASONIC_SLAVE_ID

#include <Arduino.h>
#include "Frugal-IoT.h" // For frugal_iot

Sensor_Ultrasonic::Sensor_Ultrasonic(const char* const id, const char * const name, float max, const char* color, const bool retain,
  System_RS485* bus, float offset, float scale, uint8_t slave_id, uint16_t reg)
  : Sensor_Float(id, name, 0, 0, max, color, retain), // width 0 - the register is whole millimetres
    modbus(slave_id, bus),
    offset(offset),
    scale(scale),
    reg(reg)
  { }

void Sensor_Ultrasonic::setup() {
  Sensor_Float::setup(); // Will readConfigFromFS and set offset and scale, do before the bus
  modbus.initialize();
}

// Raw distance in mm, or NAN if the slave did not answer - Sensor_Float::validate rejects NAN
// so a failed read leaves the last good value published rather than publishing a bogus one.
float Sensor_Ultrasonic::readFloat() {
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

// Defaults (offset 0, scale 1) leave the reading as the module's raw millimetres.
float Sensor_Ultrasonic::convert(float v) {
  return offset + v * scale;
}

void Sensor_Ultrasonic::dispatch(System_Message &msg) {
  // offset and scale arrive either from LittleFS at setup, or from the captive portal / MQTT
  if (msg.isSet() && (msg.module() == id) && ((msg.leaf() == "offset") || (msg.leaf() == "scale"))) {
    if (msg.leaf() == "offset") {
      offset = msg.payload.toFloat();
    } else {
      scale = msg.payload.toFloat();
    }
    msg.maybeWriteToFSandEcho(); // No-op back to the filesystem when the message came from it
    #ifdef SENSOR_ULTRASONIC_DEBUG
      Serial.print(F("Ultrasonic: offset=")); Serial.print(offset);
      Serial.print(F(" scale=")); Serial.println(scale);
    #endif
  } else {
    Sensor_Float::dispatch(msg);
  }
}

void Sensor_Ultrasonic::captiveLines(AsyncResponseStream* response) {
  Sensor_Float::captiveLines(response); // Current reading
  // Scale is deliberately not offered here - addNumber emits step=1 so a fractional scale
  // could not be entered. Set it in the constructor, over MQTT, or in /<id>/scale on LittleFS.
  frugal_iot.captive->addNumber(response, id, "offset", String(offset, 0), T->Offset, -(long)output->max, (long)output->max);
}

#endif // SENSOR_ULTRASONIC_SLAVE_ID
