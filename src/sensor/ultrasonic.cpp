/*
 * Ultrasonic distance sensor - shared base class.
 *
 * See sensor/ultrasonic.h for the offset/scale convention, and the subclasses for the hardware.
 */

#include "sensor/ultrasonic.h"

#include <Arduino.h>
#include "Frugal-IoT.h" // For frugal_iot

Sensor_Ultrasonic::Sensor_Ultrasonic(const char* const id, const char * const name, float max,
  const char* color, const bool retain, float offset, float scale)
  : Sensor_Float(id, name, 0, 0, max, color, retain), // width 0 - whole millimetres
    offset(offset),
    scale(scale)
  { }

// Defaults (offset 0, scale 1) leave the reading as raw millimetres.
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
      Serial.print(id); Serial.print(F(": offset=")); Serial.print(offset);
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
