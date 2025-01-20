/*
 * Sensor Analog
 * Read from a pin and send message
 *
 * See https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html for lots more on ESP ADCs
 *
 * Configuration options.
 * Required: SENSOR_XYZ_WANT - compiled based on any of its subclasses
 * Optional: SENSOR_ANALOG_REFERENCE for ESP8266 only  
 *
 */

#include "_settings.h"  // Settings for what to include etc
#include "sensor_analog.h" // defines SENSOR_ANALOG_WANT if needed

#ifdef SENSOR_ANALOG_WANT

#include <Arduino.h>
#include "system_mqtt.h"
#include "system_discovery.h" // 

//TO_ADD_BOARD
//  https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
// TODO what are the values on ESP8266 or ESP32
// TODO map between one set of REFERENCE values and the board specfic ones from the docs 
// See https://github.com/mitra42/frugal-iot/issues/60
// TODO Note this is not going to make it to the place its used in sensor_analog
#ifndef SENSOR_ANALOG_REFERENCE
  #ifdef ESP8266_D1
    #define SENSOR_ANALOG_REFERENCE DEFAULT // TODO not clear if / where this is used 
  #else
    #ifndef LOLIN_C3_PICO
      #error analogReference() is board dependent, review the docs and online and define 
    #endif
  #endif
#endif //  SENSOR_ANALOG_REFERENCE

Sensor_Analog::Sensor_Analog(const uint8_t p, const uint8_t smooth_init, const char* const topic_init, const unsigned long ms_init) : Sensor_Uint16(smooth_init, topic_init, ms_init), pin(p) { };

// Sensor_Uint16_t::act is good - sends with retain=false; qos=0;
// Sensor_Uint16_t::set is good - does optional smooth, compares and calls act
// Sensor_Uint16_t::loop is good - does periodic read and set

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
}
#endif // SENSOR_ANALOG_WANT

