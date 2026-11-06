/*
 * Sensor_PM - the measurement cycle shared by laser particulate sensors.
 *
 * NOTE - AS OF 2026-11-06 THIS IS UNTESTED ON HARDWARE - BUG REPORTS OR CONFIRMATION THAT IT WORKS VERY WELCOME
 *
 * See sensor/pm.h for the state machine and what a subclass provides.
 */

#include "sensor/pm.h"
#include <Arduino.h>
#include "system/frugal.h" // frugal_iot.powercontroller

Sensor_PM::Sensor_PM(const char * const id, const char * const name, const unsigned long warmup_ms,
  const unsigned long reading_ms, const bool retain)
  : Sensor(id, name, retain),
    warmup_ms(warmup_ms),
    reading_ms(reading_ms)
  { }

OUTfloat* Sensor_PM::channel(const uint8_t c) {
  OUTfloat* o = pm10;
  if (c == PM1) {
    o = pm1;
  } else if (c == PM25) {
    o = pm25;
  }
  return o;
}

void Sensor_PM::wake() {
  wakeModule();
  running = true;
}

void Sensor_PM::sleep() {
  if (sleepModule()) {
    running = false;
  }
}

// A cycle shorter than a whole measurement would wake the module again almost as soon as it slept,
// so it is left running, as airRohr does.
bool Sensor_PM::keepRunning() {
  return frugal_iot.powercontroller->cycle_ms <= (warmup_ms + reading_ms);
}

// Start of a cycle - start a measurement, unless the last one is still going (only possible when
// the cycle is shorter than a measurement, in which case it just carries on).
void Sensor_PM::periodically() {
  if (state == IDLE) {
    for (uint8_t c = 0; c < CHANNELS; c++) {
      sum[c] = 0;
      minimum[c] = 0xFFFF;
      maximum[c] = 0;
    }
    count = 0;
    phaseStart = millis(); // Not sleepSafeMillis - this is sub-cycle timing, and restarts each wake
    if (running) {
      state = READING; // Fan never stopped - nothing to warm up
    } else {
      wake();
      state = WARMING;
    }
    #ifdef SENSOR_PM_DEBUG
      Serial.printf("%s: %s\n", id, (state == WARMING) ? "waking, warming up" : "reading");
    #endif
  }
}

/* Runs on every pass of the main loop, so it must not block.
 * readSample() is called in every state, not only while READING - a streaming sensor such as the
 * SDS011 sends a sample a second while awake, and its UART buffer would overflow during a warm-up
 * if they were left there.
 */
void Sensor_PM::loop() {
  uint16_t v[CHANNELS] = { 0, 0, 0 };
  while (readSample(v)) {
    if (state == READING) {
      for (uint8_t c = 0; c < CHANNELS; c++) {
        if (channel(c)) {
          sum[c] += v[c];
          if (v[c] < minimum[c]) { minimum[c] = v[c]; }
          if (v[c] > maximum[c]) { maximum[c] = v[c]; }
        }
      }
      count++;
      #ifdef SENSOR_PM_DEBUG
        Serial.printf("%s: sample PM1=%.1f PM2.5=%.1f PM10=%.1f\n", id, v[PM1] / 10.0, v[PM25] / 10.0, v[PM10] / 10.0);
      #endif
    }
  }
  if ((state == WARMING) && ((millis() - phaseStart) >= warmup_ms)) {
    phaseStart = millis();
    state = READING;
  } else if ((state == READING) && ((millis() - phaseStart) >= reading_ms)) {
    readValidateConvertSet();
    if (!keepRunning()) {
      sleep();
    }
    state = IDLE; // Which is what lets the node sleep - see okToSleep()
  }
}

// Called from loop() at the end of the reading phase, not from periodically() as most sensors are.
void Sensor_PM::readValidateConvertSet() {
  if (count) {
    const bool trim = (count > 2); // Drop the highest and lowest, as airRohr does
    const uint8_t n = trim ? (count - 2) : count;
    for (uint8_t c = 0; c < CHANNELS; c++) {
      OUTfloat* o = channel(c);
      if (o) {
        const uint32_t s = trim ? (sum[c] - minimum[c] - maximum[c]) : sum[c];
        o->set(s / (n * 10.0));
      }
    }
  } else {
    // Nothing in the whole reading window - module missing, unpowered, or miswired
    reportNoData();
    setOutputsInvalid();
  }
  #ifdef SENSOR_PM_DEBUG
    Serial.printf("%s: %d samples", id, count);
    for (uint8_t c = 0; c < CHANNELS; c++) {
      OUTfloat* o = channel(c);
      if (o) {
        Serial.printf(" %s=%.1f", o->id, o->floatValue());
      }
    }
    Serial.println();
  #endif
}

// No side effects allowed here - see System_Base::okToSleep()
bool Sensor_PM::okToSleep(unsigned long ms, uint16_t how) {
  (void)ms; (void)how;
  return state == IDLE;
}

// Only reached if the node sleeps while the module is still running - when keepRunning() left it
// on. Under Power_Loop prepare() is never called.
void Sensor_PM::prepare() {
  if (running) {
    sleep();
  }
  Sensor::prepare();
}
