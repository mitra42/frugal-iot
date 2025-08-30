/* Frugal IoT - Soil Sensor
 *
  * Mitra Ardron: 2024
 * 
 * Tested values:
 * Device        | 0% reading | 100% reading
 * --------------|------------|-------------
 * default       | 3000       | 1000
 * Lilygo HiGrow | 4095       | 0
 * 
 * Pins by board:
 * LILYGOHIGROW: 32
 * ESP8266 D1: A0 - there is only one analog on D1
 * ARDUINO_LOLIN_C3_PICO: 4, 0 or 1 seem to work - there is no default pin
 *
 * Add by for example in main.cpp with:
 * frugal_iot.sensors->add(new Sensor_Soil("soil", "Soil", 4095, 0, 32, 0, "brown", true));
 *
 */

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "sensor_soil.h"

Sensor_Soil::Sensor_Soil(const char* const id, const char * const name, const uint16_t map0_init, const uint16_t map100_init, const uint8_t pin_init, const uint8_t smooth_init, const char* color, bool retain) 
  : Sensor_Analog(id, name, pin_init, smooth_init, 0, 100, color, retain), map0(map0_init), map100(map100_init) { }

#define SENSOR_SOIL_INVALIDVALUE 0xFFFF

uint16_t Sensor_Soil::read() {
  const uint16_t x = analogRead(pin);
  #ifdef SENSOR_SOIL_DEBUG
    Serial.print(F("Soil sensor reading:")); Serial.println(x);
  #endif
  if (x == 4095) { // 12 bit -1 i.e. 0xFFF
    return SENSOR_SOIL_INVALIDVALUE;
  }
  // TODO-85 will want to be able to calibrate this somehow and remember calibration
  return map(x, map0, map100, 0, 100);
}
bool Sensor_Soil::valid(uint16_t newvalue) {
  return (newvalue != SENSOR_SOIL_INVALIDVALUE);
}
