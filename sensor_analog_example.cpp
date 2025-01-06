/*
  Sensor Analog
  Read from a pin and return as sAnalog::Value

Configuration options
Optional: SENSOR_ANALOG_PIN SENSOR_ANALOG_MS SENSOR_ANALOG_SMOOTH SENSOR_ANALOG_EXAMPLE_TOPIC

On C3 - pin 0,1,4 works  5 gets error message  3 is Vbatt. 2 just reads 4095; 8,10 just reads 0; 7 reads 0 ad seems connected to LED

*/

// TODO turn this into a template
// TODO add the _ARRAY parameters as used in sensor_sht85.cpp so will read multiple analog inputs.

#include "_settings.h"  // Settings for what to include etc
#ifdef SENSOR_ANALOG_EXAMPLE_WANT
#define SENSOR_ANALOG_EXAMPLE_DEBUG // For now 
#include <Arduino.h>
#include "sensor_analog.h"
#include "sensor_analog_example.h"
#include "system_discovery.h"

#ifndef SENSOR_ANALOG_EXAMPLE_PIN
  #ifdef ESP8266_D1_MINI
    // Only one analog pin on D1 Mini
    #define SENSOR_ANALOG_EXAMPLE_PIN A0
  #else
    #ifdef LOLIN_C3_PICO
      // 0,1,4 work 5 gets error message; 3 is Vbatt; 2 just gets 4095, 8,10 get 0, 7 gets 0 and seems connected to LED
      #define SENSOR_ANALOG_EXAMPLE_PIN 4 // Which pin to read: 
    #else
      #error Sorry no default Analog pin for your board // #TO_ADD_BOARD
    #endif
  #endif
#endif
#ifndef SENSOR_ANALOG_EXAMPLE_MS
  #define SENSOR_ANALOG_EXAMPLE_MS 15000 // How often to read in MS - default to 15 seconds if not overridden
#endif

//  https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
// TODO what are the values on ESP8266 or ESP32
// TODO map between one set of REFERENCE values and the board specfic ones from the docs 
// See https://github.com/mitra42/frugal-iot/issues/60
#ifndef SENSOR_ANALOG_REFERENCE
  #ifdef ESP8266_D1_MINI
    #define SENSOR_ANALOG_REFERENCE DEFAULT // TODO not clear if / where this is used 
  #else
    #ifndef LOLIN_C3_PICO
      #error analogReference() is board dependent, review the docs and online and define 
    #endif
  #endif
#endif //  SENSOR_ANALOG_REFERENCE



namespace sAnalogExample {

Sensor_Analog sensor_analog_example(SENSOR_ANALOG_EXAMPLE_PIN);


void setup() {
  #ifdef SENSOR_ANALOG_EXAMPLE_DEBUG
    sensor_analog_example.name = new String(F("analog_example"));
  #endif // SENSOR_ANALOG_EXAMPLE_DEBUG
  #ifdef SENSOR_ANALOG_EXAMPLE_SMOOTH
    sensor_analog_example.smooth = SENSOR_ANALOG_EXAMPLE_SMOOTH;
  #endif
  #ifdef SENSOR_ANALOG_EXAMPLE_MS
    sensor_analog_example.ms = SENSOR_ANALOG_EXAMPLE_MS;
  #endif
  sensor_analog_example.topic = SENSOR_ANALOG_EXAMPLE_TOPIC;
  sensor_analog_example.setup();
}
// TODO create a linked list of sensors, and another of actuators that can be called in loop OR use list by time
void loop() {
  sensor_analog_example.loop();
}

} //namespace sAnalog
#endif // SENSOR_ANALOG_EXAMPLE_WANT
