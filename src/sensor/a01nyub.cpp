/*
 * DFRobot A01NYUB waterproof ultrasonic distance sensor - UART output.
 *
 * NOTE - AS OF 2026-10-13 THIS IS UNTESTED CODE AS WE DONT HAVE THE HARDWARE -
 * BUG REPORTS OR CONFIRMATION THAT IT WORKS VERY WELCOME
 *
 * See sensor/a01nyub.h for the frame format, wiring, the related modules, and the flags, and
 * sensor/ultrasonic.h for the offset/scale convention.
 */

#include "sensor/a01nyub.h"

#include <Arduino.h>
#include "cmath" // for std::isnan

Sensor_A01NYUB::Sensor_A01NYUB(const char* const id, const char * const name, HardwareSerial* serial, uint8_t rx_pin,
  float max, const char* color, const bool retain, float offset, float scale)
  : Sensor_Ultrasonic(id, name, max, color, retain, offset, scale),
    serial(serial),
    rx_pin(rx_pin)
  { }

void Sensor_A01NYUB::setup() {
  Sensor_Ultrasonic::setup(); // Sensor::setup -> readConfigFromFS, sets offset and scale
  #ifdef ESP8266
    // ESP8266 UART pins are fixed in hardware (no arbitrary rx remap like ESP32) - rx_pin is
    // ignored here, see the UART note in sensor/a01nyub.h.
    serial->begin(9600, SERIAL_8N1);
  #else
    // TX -1: nothing is sent to the module, so no pin is claimed for it. HardwareSerial only
    // falls back to the UART's default pins when BOTH are negative.
    serial->begin(9600, SERIAL_8N1, rx_pin, -1);
  #endif
  #ifdef SENSOR_A01NYUB_DEBUG
    Serial.print(id); Serial.print(F(": UART rx=")); Serial.println(rx_pin);
  #endif
}

// Distance in mm from the first frame with a good checksum that arrives after the stale buffer
// is discarded, or NAN if none did within SENSOR_A01NYUB_TIMEOUT_MS.
float Sensor_A01NYUB::readFloat() {
  const unsigned long start = millis(); // Not sleepSafeMillis as this is a sub-second wait
  while (serial->available()) {
    serial->read();
  }
  uint8_t frame[4]; // Sliding window - frame[0] is the oldest byte
  uint8_t n = 0;    // Bytes in the window
  float mm = NAN;
  while (std::isnan(mm) && ((millis() - start) < SENSOR_A01NYUB_TIMEOUT_MS)) {
    if (serial->available()) {
      const uint8_t b = serial->read();
      if (n < 4) {
        frame[n++] = b;
      } else {
        frame[0] = frame[1]; frame[1] = frame[2]; frame[2] = frame[3]; frame[3] = b;
      }
      if ((n == 4) && (frame[0] == 0xFF) && (((frame[0] + frame[1] + frame[2]) & 0xFF) == frame[3])) {
        mm = (frame[1] << 8) | frame[2];
      }
    } else {
      delay(1); // Nothing buffered - let the idle task run rather than spin
    }
  }
  #ifdef SENSOR_A01NYUB_DEBUG
    Serial.print(id); Serial.print(F(": ")); Serial.print(mm);
    Serial.print(F("mm after ")); Serial.print(millis() - start); Serial.println(F("ms"));
  #endif
  return mm;
}

bool Sensor_A01NYUB::validate(float mm) {
  return Sensor_Ultrasonic::validate(mm) && (mm >= SENSOR_A01NYUB_MIN_MM) && (mm <= SENSOR_A01NYUB_MAX_MM);
}
