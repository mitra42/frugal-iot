/*
  Sensor Analog
  Read from a pin and return as sAnalog::Value

Configuration options
Optional: SENSOR_ANALOG_EXAMPLE_PIN SENSOR_ANALOG_EXAMPLE_MS SENSOR_ANALOG_EXAMPLE_SMOOTH SENSOR_ANALOG_EXAMPLE_TOPIC
*/

#include "_settings.h"  // Settings for what to include etc
#ifdef SENSOR_ANALOG_EXAMPLE_WANT
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

#ifndef SENSOR_ANALOG_EXAMPLE_SMOOTH
  #define SENSOR_ANALOG_EXAMPLE_SMOOTH 0
#endif

//TODO-23 for power management will rethink individual timers
#ifndef SENSOR_ANALOG_EXAMPLE_MS
  #define SENSOR_ANALOG_EXAMPLE_MS 15000 // How often to read in MS - default to 15 seconds if not overridden
#endif

Sensor_Analog sensor_analog_example(SENSOR_ANALOG_EXAMPLE_PIN, SENSOR_ANALOG_EXAMPLE_SMOOTH, SENSOR_ANALOG_EXAMPLE_TOPIC, SENSOR_ANALOG_EXAMPLE_MS);

// No need for setup or loop - will be called by Sensor::setupAll and Sensor::loopAll

#endif // SENSOR_ANALOG_EXAMPLE_WANT
