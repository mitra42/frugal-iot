/*
 * HC-SR04 ultrasonic distance sensor - trigger / echo timing.
 *
 * NOTE - AS OF 2026-09-28 THIS IS UNTESTED CODE
 *
 * See sensor/hcsr04.h for the wiring (ECHO is 5V - use a divider), the modules it supports and the
 * settings for each (HC-SR04, AJ-SR04M), the flags, and why this does not use robtillaart/SRF05.
 */

#include "sensor/hcsr04.h"

#include <Arduino.h>
#include "Frugal-IoT.h" // For frugal_iot
#include "cmath" // for std::isnan

Sensor_HCSR04::Sensor_HCSR04(const char* const id, const char * const name, uint8_t trigger_pin, uint8_t echo_pin,
  float max, const char* color, const bool retain, float offset, float scale)
  : Sensor_Ultrasonic(id, name, max, color, retain, offset, scale),
    // Not in the schema, like Sensor_DissolvedOxygen's water temperature - hence the literal colour
    temperature(new INfloat(id, "temperature", "Air Temperature",
      SENSOR_HCSR04_DEFAULT_TEMPERATURE, 1, -20, 50, "red", true)),
    trigger_pin(trigger_pin),
    echo_pin(echo_pin)
  { }

void Sensor_HCSR04::setup() {
  temperature->setup();         // Before readConfigFromFS, which may carry a stored wired path
  Sensor_Ultrasonic::setup();   // Sensor::setup -> readConfigFromFS, sets offset and scale
  pinMode(trigger_pin, OUTPUT);
  digitalWrite(trigger_pin, LOW);
  pinMode(echo_pin, INPUT);
  #ifdef SENSOR_HCSR04_TEMPERATURE_PATH
    // Only apply the compile-time default if nothing on the filesystem or in the UX wired it
    if (!temperature->wiredPath.length()) {
      temperature->wireTo(frugal_iot.messages->path(SENSOR_HCSR04_TEMPERATURE_PATH));
    }
  #endif
  #ifdef SENSOR_HCSR04_DEBUG
    Serial.print(id); Serial.print(F(": trigger=")); Serial.print(trigger_pin);
    Serial.print(F(" echo=")); Serial.print(echo_pin);
    Serial.print(F(" temperature wired to ")); Serial.println(temperature->wiredPath);
  #endif
}

void Sensor_HCSR04::discover() {
  Sensor_Ultrasonic::discover(); // Outputs
  temperature->discover();
}

void Sensor_HCSR04::dispatch(System_Message &msg) {
  // The wired temperature is published by *another* module, so this has to happen outside any
  // msg.module() == id test - compare Sensor_DissolvedOxygen::dispatch(). Nothing to do with
  // the result here: the value is read when it is needed, in speedOfSound().
  temperature->dispatch(msg);
  Sensor_Ultrasonic::dispatch(msg);
}

// One round trip, in microseconds, or 0 if no echo arrived within SENSOR_HCSR04_TIMEOUT_US.
// With nothing in range the module holds ECHO high for about 38ms, longer than the timeout, and
// pulseIn returns 0 for that as well.
uint32_t Sensor_HCSR04::ping() {
  digitalWrite(trigger_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigger_pin, HIGH);
  delayMicroseconds(SENSOR_HCSR04_TRIGGER_US);
  digitalWrite(trigger_pin, LOW);
  return pulseIn(echo_pin, HIGH, SENSOR_HCSR04_TIMEOUT_US);
}

float Sensor_HCSR04::speedOfSound() {
  float t = temperature->floatValue();
  if (std::isnan(t)) { // The wired sensor published "nan"
    t = SENSOR_HCSR04_DEFAULT_TEMPERATURE;
  }
  return 331.3 + 0.606 * t;
}

float Sensor_HCSR04::readFloat() {
  uint32_t echoes[SENSOR_HCSR04_SAMPLES]; // Kept sorted as they arrive, for the median
  uint8_t n = 0;
  for (uint8_t i = 0; i < SENSOR_HCSR04_SAMPLES; i++) {
    if (i) {
      delay(SENSOR_HCSR04_INTERVAL_MS); // Not sleepSafeMillis as this is a sub-second wait
    }
    const uint32_t us = ping();
    if (us) { // Timeouts are dropped, not averaged in as zeros
      uint8_t j = n++;
      while (j && (echoes[j - 1] > us)) {
        echoes[j] = echoes[j - 1];
        j--;
      }
      echoes[j] = us;
    }
  }
  float mm = NAN;
  if (n) {
    const float median_us = (n & 1) ? echoes[n / 2] : (echoes[n / 2 - 1] + echoes[n / 2]) / 2.0;
    mm = median_us * speedOfSound() / 2000.0; // Round trip, so half; m/s * us = um, /1000 = mm
  }
  #ifdef SENSOR_HCSR04_DEBUG
    Serial.print(id); Serial.print(F(": ")); Serial.print(n); Serial.print(F("/"));
    Serial.print(SENSOR_HCSR04_SAMPLES); Serial.print(F(" echoes, median "));
    Serial.print(mm); Serial.print(F("mm at ")); Serial.print(speedOfSound()); Serial.println(F("m/s"));
  #endif
  return mm;
}

bool Sensor_HCSR04::validate(float mm) {
  return Sensor_Ultrasonic::validate(mm) && (mm >= SENSOR_HCSR04_MIN_MM) && (mm <= SENSOR_HCSR04_MAX_MM);
}
