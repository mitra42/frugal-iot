/*
 * Nova Fitness SDS011 laser particulate sensor - PM2.5 and PM10 over a UART.
 *
 * NOTE - AS OF 2026-11-05 THIS IS UNTESTED CODE - BUG REPORTS OR CONFIRMATION THAT IT WORKS VERY WELCOME
 *
 * See sensor/sds011.h for the protocol, the timing, the wiring and the flags.
 */

#include "sensor/sds011.h"

#ifdef ESP32 // See sds011.h - ESP8266 has no UART free to receive on

#include <Arduino.h>

extern unsigned long wake_count; // system/power.cpp - zero on a power-on, non-zero on a deep-sleep wake

Sensor_SDS011::Sensor_SDS011(const char * const name, HardwareSerial* serial, uint8_t rx_pin, uint8_t tx_pin, bool retain)
  : Sensor_PM("sds011", name, SENSOR_SDS011_WARMUP_MS, SENSOR_SDS011_READING_MS, retain),
    serial(serial),
    rx_pin(rx_pin),
    tx_pin(tx_pin)
  {
  outputs.push_back(pm25 = new OUTfloat("sds011", "pm25", "PM2.5", 0, 1, IO_DEFAULTS(sds011_pm25), false));
  outputs.push_back(pm10 = new OUTfloat("sds011", "pm10", "PM10", 0, 1, IO_DEFAULTS(sds011_pm10), false));
  pm25->unit = "ug/m3";
  pm10->unit = "ug/m3";
}

void Sensor_SDS011::setup() {
  Sensor::setup();
  serial->begin(9600, SERIAL_8N1, rx_pin, tx_pin);
  #ifdef SENSOR_SDS011_DEBUG
    Serial.printf("%s: UART rx=%d tx=%d\n", id, rx_pin, tx_pin);
  #endif
  if (!wake_count) {
    /* A power-on, not a deep-sleep wake. Make sure the module sends frames by itself and does not
     * duty-cycle itself - something else (a previous firmware, a PC utility) may have changed
     * either, and the module remembers both across power cycles. Not done on every wake as each
     * is a write to the module's own flash.
     * The module may have been left asleep, so wake it first - but not through wake(), which would
     * set `running` and so make the first measurement skip its warm-up. `running` stays false:
     * the module is told to sleep at the end of each measurement, so it never runs through a sleep.
     */
    command(0x06, 0x01, 0x01);
    command(0x02, 0x01, 0x00); // Active reporting
    command(0x08, 0x01, 0x00); // Continuous - no work period of its own
  }
}

// One 19 byte command frame - see sds011.h. Device id FF FF means "whichever module is there".
void Sensor_SDS011::command(const uint8_t cmd, const uint8_t d1, const uint8_t d2) {
  uint8_t buf[19] = { 0xAA, 0xB4, cmd, d1, d2, 0,0,0,0,0,0,0,0,0,0, 0xFF, 0xFF, 0, 0xAB };
  uint8_t sum = 0;
  for (uint8_t i = 2; i < 17; i++) {
    sum += buf[i];
  }
  buf[17] = sum;
  serial->write(buf, sizeof(buf));
  serial->flush(); // Out of the UART before anything - deep sleep included - can stop it
}

void Sensor_SDS011::wakeModule() {
  command(0x06, 0x01, 0x01);
}

bool Sensor_SDS011::sleepModule() {
  command(0x06, 0x01, 0x00);
  return true;
}

// Slide each buffered byte into a ten byte window; true as soon as the window holds a good frame,
// with its readings put in v. Bytes after it stay in the UART for the next call.
bool Sensor_SDS011::readSample(uint16_t* v) {
  bool good = false;
  while (!good && serial->available()) {
    const uint8_t b = serial->read();
    if (frameLen < sizeof(frame)) {
      frame[frameLen++] = b;
    } else {
      memmove(frame, frame + 1, sizeof(frame) - 1);
      frame[sizeof(frame) - 1] = b;
    }
    if ((frameLen == sizeof(frame)) && (frame[0] == 0xAA) && (frame[1] == 0xC0) && (frame[9] == 0xAB)) {
      uint8_t sum = 0;
      for (uint8_t i = 2; i < 8; i++) {
        sum += frame[i];
      }
      if (sum == frame[8]) {
        good = true;
        frameLen = 0; // Consumed - so the same frame is not matched again
        v[PM25] = frame[2] | (frame[3] << 8); // Tenths of ug/m3, little-endian - as Sensor_PM wants
        v[PM10] = frame[4] | (frame[5] << 8);
      }
    }
  }
  return good;
}

void Sensor_SDS011::reportNoData() {
  Serial.printf("%s: no data from the SDS011 - check its 5V and that its TX goes to pin %d\n", id, rx_pin);
}

#endif // ESP32
