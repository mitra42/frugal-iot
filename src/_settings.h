/*
 * This is a place to configure all aspects of the compilation
 * Can add sensor-specific settings here, also those needed to control imported libraries
 * Note common structures etc go in _common.ini
 * This file will be built automatically from something in YAML
 * OR there will be part auto-built and that will be included from here
 */
#ifndef _SETTINGS_H
#define _SETTINGS_H

#include "_local.h"               // Will fail if user hasn't copied _local-template.h to _local.h and edited

// TO_ADD_SENSOR - add in appropriate line below depending on superclass
#if defined(SENSOR_SHT_WANT) || defined(SENSOR_DHT_WANT)
  #define SENSOR_HT_WANT
#endif
#if defined(SENSOR_SHT_DEBUG) || defined(SENSOR_DHT_DEBUG)
  #define SENSOR_HT_DEBUG
#endif
#if defined(SENSOR_ANALOG_INSTANCES_WANT) || defined(SENSOR_BATTERY_WANT) || defined(SENSOR_SOIL_WANT)
  #define SENSOR_ANALOG_WANT
#endif
#if defined(SENSOR_ANALOG_INSTANCES_DEBUG) || defined(SENSOR_BATTERY_DEBUG) || defined(SENSOR_SOIL_DEBUG)
  #define SENSOR_ANALOG_DEBUG
#endif
#if defined(SENSOR_ANALOG_WANT) 
  #define SENSOR_UINT16_WANT
#endif
#if defined(SENSOR_ANALOG_DEBUG) 
  #define SENSOR_UINT16_DEBUG
#endif
#if defined(SENSOR_BH1750_WANT)
  #define SENSOR_FLOAT_WANT
#endif
#if defined(SENSOR_BH1750_DEBUG)
  #define SENSOR_FLOAT_DEBUG
#endif
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

// TO_ADD_CONTROL
#if defined(CONTROL_BLINKEN_DEBUG) || defined(CONTROL_HYSTERISIS_DEBUG)
  #define CONTROL_DEBUG
#endif

#if defined(CONTROL_BLINKEN_WANT) || defined(CONTROL_HYSTERISIS_WANT)
  #define CONTROL_WANT
#endif

// TO_ADD_SYSTEM - there is no class hierarchy
#if defined(SYSTEM_WIFI_DEBUG) || defined(SYSTEM_MQTT_DEBUG) || defined(SYSTEM_DISCOVERY_DEBUG) || defined(SYSTEM_OTA_DEBUG)
  #define SYSTEM_DEBUG
#endif

#if defined(SENSOR_DEBUG) || defined(ACTUATOR_DEBUG) || defined(CONTROL_DEBUG) || defined(SYSTEM_DEBUG)
  #define ANY_DEBUG
#endif 

#if defined(SENSOR_MS5803_SPI) 
  #define SENSOR_SPI_WANT
#endif

// TO_ADD_BOARD
// shields compatible with D1 and its ESP8266 not C-pico which has same pin layout but different availability esp of analog
#if defined(ESP8266_D1_MINI_PROv2) || defined (ESP8266_D1_MINI) || defined(ESP8266_D1_PRO_CLONE)
  #define ESP8266_D1
#endif

// To specify a language (for the WiFi portal) #define all the ones you want, otherwise it supports the _ALL lsit which is currerntly EN, NL, DE, ID 
#if \
   !defined LANGUAGE_EN \
&& !defined LANGUAGE_NL \
&& !defined LANGUAGE_DE \
&& !defined LANGUAGE_ID
    #define LANGUAGE_ALL
#endif

#define SERIAL_DELAY 5000 // Necessary to avoid losing initial messsage in garbage, at least on ESP8266_D1_MINI

// Always defined currently, but recommend defining in _locals.h in case that decision ever changes
#define SYSTEM_DISCOVERY_WANT // Almost always set, will tell the MQTT server about the device so the Client can find it 
#define SERIAL_BAUD 460800 // Generally find 460800 works well - reliability on all boards tested
#define SYSTEM_WIFI_WANT  // currently always wanted - recommend defining in _locals.h in case that decision ever changes
#define SYSTEM_MQTT_WANT // Given the dependence on MQTT can't imagine not "wanting" it
 #endif // _SETTINGS_H
