/*
  Sensor Analog
  Read from a pin and return as sAnalog::Value

Configuration options
Optional: SENSOR_ANALOG_PIN SENSOR_ANALOG_MS SENSOR_ANALOG_SMOOTH SENSOR_ANALOG_EXAMPLE_TOPIC

*/

// TODO turn this into a template
// TODO add the _ARRAY parameters as used in sensor_sht85.cpp so will read multiple analog inputs.

#include "_settings.h"  // Settings for what to include etc
#ifdef SENSOR_ANALOG_EXAMPLE_WANT
#include <Arduino.h>
#include "sensor_analog.h"
#include "sensor_analog_example.h"
#include "system_discovery.h"

#ifndef SENSOR_ANALOG_EXAMPLE_PIN
  #ifdef LOLIN_C3_PICO
    #define SENSOR_ANALOG_EXAMPLE_PIN A0 // Which pin to read - this might be board specific
  #else
    #error Sorry no default Analog pin for your board
  #endif
#endif
#ifndef SENSOR_ANALOG_EXAMPLE_MS
  #define SENSOR_ANALOG_EXAMPLE_MS 1000 // How often to read in MS
#endif



namespace sAnalogExample {

Sensor_Analog sensor_analog_example(SENSOR_ANALOG_EXAMPLE_PIN);


void setup() {
  #ifdef SENSOR_ANALOG_EXAMPLE_DEBUG
    sensor_analog_example.name = new String(F("analog_example"));
  #endif // SENSOR_ANALOG_EXAMPLE_DEBUG
  #ifdef SENSOR_ANALOG_SMOOTH
    sensor_analog_example.smooth = SENSOR_ANALOG_SMOOTH;
  #endif
  sensor_analog_example.topic = String(*xDiscovery::topicPrefix + SENSOR_ANALOG_EXAMPLE_TOPIC);
  sensor_analog_example.setup();
}
// TODO create a linked list of sensors, and another of actuators that can be called in loop OR use list by time
void loop() {
  sensor_analog_example.loop();
}

} //namespace sAnalog
#endif // SENSOR_ANALOG_EXAMPLE_WANT
