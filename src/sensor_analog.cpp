/*
 * Sensor Analog
 * Read from a pin and send message
 *
 * See https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html for lots more on ESP ADCs
 *
 * Configuration options.
 * Optional: SENSOR_ANALOG_REFERENCE for ESP8266 only  // TODO-141 phase out
 * Optional: SENSOR_ANALOG_ATTENTUATION // TODO-141 phase out
 * TODO: There is a lot more clever stuff on https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html
 * Its ESP32 specific, but looks like a range of capabilities that could be integrated.
 * 
 * On C3 - pin 0,1,4 works  5 gets error message  3 is Vbatt. 2 just reads 4095; 8,10 just reads 0; 7 reads 0 ad seems connected to LED
 */

#include "_settings.h"  // Settings for what to include etc
#include "sensor_analog.h"


#include <Arduino.h>

//TO_ADD_BOARD
//  https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
// TODO what are the values on ESP8266 or ESP32
// TODO map between one set of REFERENCE values and the board specfic ones from the docs 
// See https://github.com/mitra42/frugal-iot/issues/60
// TODO Note this is not going to make it to the place its used in sensor_analog
#ifndef SENSOR_ANALOG_REFERENCE
  #ifdef ESP8266_D1
    #define SENSOR_ANALOG_REFERENCE DEFAULT // TODO not clear if / where this is used 
  #elif defined(ESP32) // It doesnt seem to be used on ESP32s 
  #else
    #error analogReference() is processor dependent, review the docs and online and define
  #endif
#endif //  SENSOR_ANALOG_REFERENCE

Sensor_Analog::Sensor_Analog(const char* const id, const char * const name, const uint8_t p, const uint8_t smooth_init, const uint16_t min, const uint16_t max, const char* color, bool r) 
: Sensor_Uint16(id, name, smooth_init, min, max, color, r), pin(p) { };

// Sensor_Uint16_t::act is obsolete
// Sensor_Uint16_t::set is good - does optional smooth, compares and sends
// Sensor_Uint16_t::periodically is good - does periodic read and set

// Note this is virtual, and subclassed in Sensor_Battery
uint16_t Sensor_Analog::read() {
  return analogRead(pin);
}

void Sensor_Analog::setup() {

  // initialize the analog pin as an input.
  pinMode(pin, INPUT); // I don't think this is needed ?
  #ifdef SENSOR_ANALOG_REFERENCE
    analogReference(SENSOR_ANALOG_REFERENCE); // TODO see TODO's in the sensor_analog.h
  #endif 
  #ifdef SENSOR_ANALOG_ATTENTUATION
    analogSetAttentuation(SENSOR_ANALOG_ATTENTUATION)
  #endif
}
// SensorAnalog::dispatchTwig is not needed

