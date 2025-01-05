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

// Note this is empty - no functions,  look at the .h for definition of parameters 

// No need for setup or loop - will be called by Sensor::setupAll and Sensor::loopAll

#endif // SENSOR_ANALOG_EXAMPLE_WANT
