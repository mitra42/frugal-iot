/* Frugal IoT - Rain Sensor
 *
 * See sensor/rain.h for the accumulation approach and its limits.
 */

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "sensor/rain.h"
#include "Frugal-IoT.h" // For frugal_iot

Sensor_Rain::Sensor_Rain(const char* const id, const char * const name, uint8_t p, float max, float s, const char* color, bool r)
  : Sensor_Float(id, name, 2, DEFAULT_rain_rain_min, max, DEFAULT_rain_rain_min, max, color, r),
    pin(p),
    scale(s),
    accumulated(0.0),
    dry_baseline(0)
  {
    setDefaultColor(DEFAULT_rain_rain_color);
  }

void Sensor_Rain::setup() {
  Sensor_Float::setup(); // Will readConfigFromFS and set dry_baseline and scale if persisted
  pinMode(pin, INPUT);
  if (!dry_baseline) { // Not read from FS and never tared - assume dry at first boot
    dry_baseline = analogRead(pin);
  }
  last_read_ms = frugal_iot.powercontroller->sleepSafeMillis();
}

float Sensor_Rain::readFloat() {
  const int raw = analogRead(pin); // Dry reads near ADC max, wetter reduces it
  const unsigned long now = frugal_iot.powercontroller->sleepSafeMillis();
  const float dt_secs = (now - last_read_ms) / 1000.0;
  last_read_ms = now;
  const int wetness = dry_baseline - raw; // Assumed linear with rainfall rate - see rain.h
  if (wetness > 0) {
    accumulated += wetness * dt_secs;
  }
  const float mm = accumulated * scale;
  #ifdef SENSOR_RAIN_DEBUG
    Serial.print(id); Serial.print(F(" raw:")); Serial.print(raw); Serial.print(F(" dry_baseline:")); Serial.print(dry_baseline);
    Serial.print(F(" dt:")); Serial.print(dt_secs); Serial.print(F(" accumulated:")); Serial.print(accumulated);
    Serial.print(F(" mm:")); Serial.println(mm);
  #endif
  return mm;
}

void Sensor_Rain::tare() {
  dry_baseline = analogRead(pin);
  accumulated = 0.0;
  #ifdef SENSOR_RAIN_DEBUG
    Serial.print(F("Rain tare: dry_baseline=")); Serial.println(dry_baseline);
  #endif
}

void Sensor_Rain::calibrate(const float mm) {
  if (accumulated != 0.0) { // Avoid divide by zero if calibrate is called before any rain has accumulated
    scale = mm / accumulated;
  }
  #ifdef SENSOR_RAIN_DEBUG
    Serial.print(F("Rain calibrate: scale=")); Serial.println(scale);
  #endif
}

void Sensor_Rain::dispatch(System_Message &msg) {
  if (msg.module() == id) {
    // Set by UX - "Tare" is output=0  Calibrate is output=XX (known mm over the accumulated period)
    if (msg.leaf() == "output") {
      if (msg.payload.toFloat() == 0.0) {
        tare();
        writeConfigToFS("dry_baseline", String(dry_baseline));
      } else {
        calibrate(msg.payload.toFloat());
        writeConfigToFS("scale", String(scale));
      }
    // dry_baseline and scale should only be seen when reading from disk
    } else if (msg.leaf() == "dry_baseline") {
      dry_baseline = msg.payload.toInt();
    } else if (msg.leaf() == "scale") {
      scale = msg.payload.toFloat();
    } else {
      Sensor::dispatch(msg);
    }
  }
}

void Sensor_Rain::captiveLines(AsyncResponseStream* response) {
  frugal_iot.captive->addButton(response, id, "output", "0", T->Tare);
  frugal_iot.captive->addNumber(response, id, "output", String(output->floatValue(),3), T->Calibrate, 0, output->max);
}
