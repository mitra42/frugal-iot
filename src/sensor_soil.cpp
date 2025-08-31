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
 * frugal_iot.sensors->add(new Sensor_Soil("soil", "Soil",3, 4095, -100.0/4095, "brown", true));
 *
 */

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "sensor_soil.h"
#include "frugal_iot.h"

Sensor_Soil::Sensor_Soil(const char* const id, const char * const name, uint8_t pin_init, int offset, float scale, const char* color, bool retain) 
  : Sensor_Analog(id, name, pin_init, 0, 0, 100, offset, scale, color, retain)
  { }

// This may be specific to device being read - expect it to be subclassed
bool Sensor_Soil::validRaw(int v) {
  return (v != 4095);   
}

