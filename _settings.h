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
  || defined(SENSOR_ANALOG_EXAMPLE_DEBUG) || defined(SENSOR_BATTERY_DEBUG) || defined(SENSOR_DHT_DEBUG) \
  || defined(SENSOR_SHT_DEBUG) || defined(SENSOR_SOIL_DEBUG) \
  || defined(SYSTEM_WIFI_DEBUG) || defined(SYSTEM_MQTT_DEBUG) || defined(SYSTEM_DISCOVERY_DEBUG) )
  #define ANY_DEBUG // If this is commented out, the serial port will not be setup for debugging.
#endif // any *_DEBUG

// TODO_ADD_SENSOR
#if defined(SENSOR_SHT_WANT) || defined(SENSOR_DHT_WANT)
  #define SENSOR_HT_WANT
#endif
#if defined(SENSOR_ANALOG_EXAMPLE_WANT) || defined(SENSOR_BATTERY_WANT) || defined(SENSOR_SOIL_WANT)
  #define SENSOR_ANALOG_WANT
#endif
#if defined(SENSOR_ANALOG_WANT) 
  #define SENSOR_UINT16_WANT
#endif
#if defined(SENSOR_UINT16_WANT) || defined(SENSOR_FLOAT_WANT) || defined(SENSOR_HT_WANT)
  #define SENSOR_WANT
#endif

//  TODO_ADD_ACTUATOR
#if defined(ACTUATOR_RELAY_WANT) || defined(ACTUATOR_LEDBUILTIN_WANT)
  #define ACTUATOR_DIGITAL_WANT
#endif
#if defined(ACTUATOR_DIGITAL_WANT)
  #define ACTUATOR_WANT
#endif

// TODO_ADD_SYSTEM
#if defined(SYSTEM_SD_WANT) || defined(SYSTEM_SPIFFS_WANT)
  #define SYSTEM_FS_WANT
#endif

#endif // _SETTINGS_H
