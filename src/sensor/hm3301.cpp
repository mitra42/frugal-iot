/*
 * Seeed HM3301 laser particulate sensor - PM1.0, PM2.5 and PM10 over I2C.
 *
 * NOTE - AS OF 2026-11-06 THIS IS UNTESTED CODE - BUG REPORTS OR CONFIRMATION THAT IT WORKS VERY WELCOME
 *
 * See sensor/hm3301.h for the protocol, the timing and the flags, and sensor/pm.h for the cycle.
 */

#include "sensor/hm3301.h"
#include <Arduino.h>

extern unsigned long wake_count; // system/power.cpp - zero on a power-on, non-zero on a deep-sleep wake

Sensor_HM3301::Sensor_HM3301(const char * const name, uint8_t address, TwoWire* wire, bool retain)
  : Sensor_PM("hm3301", name, SENSOR_HM3301_WARMUP_MS, SENSOR_HM3301_READING_MS, retain),
    interface(address, wire)
  {
  outputs.push_back(pm1 = new OUTfloat("hm3301", "pm1", "PM1.0", 0, 1, IO_DEFAULTS(hm3301_pm1), false));
  outputs.push_back(pm25 = new OUTfloat("hm3301", "pm25", "PM2.5", 0, 1, IO_DEFAULTS(hm3301_pm25), false));
  outputs.push_back(pm10 = new OUTfloat("hm3301", "pm10", "PM10", 0, 1, IO_DEFAULTS(hm3301_pm10), false));
  pm1->unit = "ug/m3";
  pm25->unit = "ug/m3";
  pm10->unit = "ug/m3";
}

void Sensor_HM3301::setup() {
  Sensor::setup(); // Will readConfigFromFS - do before touching the device
  interface.initialize();
  interface.send(0x88); // Select I2C - harmless if already selected, so sent on every wake
  // It cannot sleep, so after a deep-sleep wake its fan has been running all along - see hm3301.h
  running = (wake_count != 0);
  #ifdef SENSOR_HM3301_DEBUG
    Serial.printf("%s: I2C 0x%02x %s\n", id, interface.addr, interface.isPresent() ? "found" : "NOT FOUND");
  #endif
}

// The HM3301 has no sleep - its fan runs whenever it has power
void Sensor_HM3301::wakeModule() { }

bool Sensor_HM3301::sleepModule() {
  return false; // Could not - so `running` stays true, and the next measurement needs no warm-up
}

// Polled, unlike the SDS011, so paced here - and only while READING, as a sample during the warm-up
// would be thrown away anyway.
bool Sensor_HM3301::readSample(uint16_t* v) {
  bool good = false;
  if ((state == READING) && ((millis() - lastSample) >= SENSOR_HM3301_SAMPLE_MS)) {
    lastSample = millis();
    uint8_t buf[29];
    if (interface.read(buf, sizeof(buf))) {
      uint8_t sum = 0;
      for (uint8_t i = 0; i < 28; i++) {
        sum += buf[i];
      }
      // An all-zero block passes the checksum, so check for that too - it is what a bus with
      // nothing answering can return
      if ((sum == buf[28]) && (buf[28] || buf[2] || buf[3])) {
        good = true;
        // Atmospheric values, whole ug/m3 big-endian - Sensor_PM wants tenths
        v[PM1]  = ((buf[10] << 8) | buf[11]) * 10;
        v[PM25] = ((buf[12] << 8) | buf[13]) * 10;
        v[PM10] = ((buf[14] << 8) | buf[15]) * 10;
      }
      #ifdef SENSOR_HM3301_DEBUG
        if (!good) {
          Serial.printf("%s: bad block, checksum %02x expected %02x\n", id, buf[28], sum);
        }
      #endif
    }
  }
  return good;
}

void Sensor_HM3301::reportNoData() {
  Serial.printf("%s: no data from the HM3301 - check its power and that it is on the I2C bus at 0x%02x\n",
    id, interface.addr);
}
