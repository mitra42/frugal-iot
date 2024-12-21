/*
 * This is a place to configure all aspects of the compilation
 * Can add sensor-specific settings here, also those needed to control imported libraries
 * Note common structures etc go in _common.ini
 * This file will be built automatically from something in YAML
 * OR there will be part auto-built and that will be included from here
 */
#ifndef _SETTINGS_H
#define _SETTINGS_H

#include "_configuration.h"      // Include (eventually) auto-generated file, defines _WANT and parameters for each module"
#include "_local.h"               // Will fail if user hasn't copied _local-template.h to _local.h and edited

// TO_ADD_ACTUATOR TO_ADD_SENSOR
#if ( defined(ACTUATOR_LEDBUILTIN_DEBUG) || defined(ACTUATOR_RELAY_DEBUG) \
  || defined(CONTROL_BLINKEN_DEBUG) || defined(CONTROL_DEMO_MQTT_DEBUG) \
  || defined(SENSOR_ANALOG_EXAMPLE_DEBUG) || defined(SENSOR_BATTERY_DEBUG) || defined(SENSOR_SHT85_DEBUG) || defined(SENSOR_DHT_DEBUG) \
  || defined(SYSTEM_WIFI_DEBUG) || defined(SYSTEM_MQTT_DEBUG) || defined(SYSTEM_DISCOVERY_DEBUG) )
  #define ANY_DEBUG // If this is commented out, the serial port will not be setup for debugging.
#endif // any *_DEBUG

#endif // _SETTINGS_H
