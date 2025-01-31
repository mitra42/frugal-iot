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


// TO_ADD_SENSOR - add in appropriate line below depending on superclass
#if defined(SENSOR_SHT_WANT) || defined(SENSOR_DHT_WANT)
  #define SENSOR_HT_WANT
#endif
#if defined(SENSOR_SHT_DEBUG) || defined(SENSOR_DHT_DEBUG)
  #define SENSOR_HT_DEBUG
#endif
#if defined(SENSOR_ANALOG_EXAMPLE_WANT) || defined(SENSOR_BATTERY_WANT) || defined(SENSOR_SOIL_WANT)
  #define SENSOR_ANALOG_WANT
#endif
#if defined(SENSOR_ANALOG_EXAMPLE_DEBUG) || defined(SENSOR_BATTERY_DEBUG) || defined(SENSOR_SOIL_DEBUG)
  #define SENSOR_ANALOG_DEBUG
#endif
#if defined(SENSOR_ANALOG_WANT) 
  #define SENSOR_UINT16_WANT
#endif
#if defined(SENSOR_ANALOG_DEBUG) 
  #define SENSOR_UINT16_DEBUG
#endif
//#if defined(SENSOR_XYZ_DEBUG) || defined(SENSOR_MPQ_DEBUG)  // THere aren't any float sensors yet
//  #define SENSOR_FLOAT_DEBUG
//#endif
#if defined(SENSOR_UINT16_WANT) || defined(SENSOR_FLOAT_WANT) || defined(SENSOR_HT_WANT) || defined(SENSOR_FLOAT_DEBUG)
  #define SENSOR_WANT
#endif
#if defined(SENSOR_UINT16_DEBUG) || defined(SENSOR_FLOAT_DEBUG) || defined(SENSOR_HT_DEBUG)
  #define SENSOR_DEBUG
#endif



//  TO_ADD_ACTUATOR - add in appropriate line below depending on superclass
#if defined(ACTUATOR_RELAY_WANT) || defined(ACTUATOR_LEDBUILTIN_WANT)
  #define ACTUATOR_DIGITAL_WANT
#endif
#if defined(ACTUATOR_RELAY_DEBUG) || defined(ACTUATOR_LEDBUILTIN_DEBUG)
  #define ACTUATOR_DIGITAL_DEBUG
#endif
#if defined(ACTUATOR_DIGITAL_WANT)
  #define ACTUATOR_WANT
#endif
#if defined(ACTUATOR_DIGITAL_DEBUG)
  #define ACTUATOR_DEBUG
#endif

// TO_ADD_CONTROL - there is not, yet, a class hierarchy - coming soon
#if defined(CONTROL_BLINKEN_DEBUG) || defined(CONTROL_DEMO_MQTT_DEBUG)
  #define CONTROL_DEBUG
#endif

// TO_ADD_SYSTEM - there is no class hierarchy
#if defined(SYSTEM_WIFI_DEBUG) || defined(SYSTEM_MQTT_DEBUG) || defined(SYSTEM_DISCOVERY_DEBUG) || defined(SYSTEM_OTA_DEBUG)
  #define SYSTEM_DEBUG
#endif

#if defined(SENSOR_DEBUG) || defined(ACTUATOR_DEBUG) || defined(CONTROL_DEBUG) || defined(SYSTEM_DEBUG)
  #define ANY_DEBUG
#endif 



// TO_ADD_BOARD
// shields compatible with D1 and its ESP8266 not C-pico which has same pin layout but different availability esp of analog
#if defined(ESP8266_D1_MINI_PROv2) || defined (ESP8266_D1_MINI) || defined(ESP8266_D1_PRO_CLONE)
  #define ESP8266_D1
#endif

#endif // _SETTINGS_H
