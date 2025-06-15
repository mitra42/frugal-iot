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
#if defined(SENSOR_BH1750_WANT) || defined(SENSOR_LOADCELL_WANT)
  #define SENSOR_FLOAT_WANT
#endif
#if defined(SENSOR_BH1750_DEBUG)
  #define SENSOR_FLOAT_DEBUG
#endif
#if defined(SENSOR_UINT16_WANT) || defined(SENSOR_FLOAT_WANT) || defined(SENSOR_HT_WANT) || defined(SENSOR_FLOAT_DEBUG) || defined(SENSOR_MS5803_WANT) || defined(SENSOR_ENSAHT_WANT)
  #define SENSOR_WANT
#endif
#if defined(SENSOR_UINT16_DEBUG) || defined(SENSOR_FLOAT_DEBUG) || defined(SENSOR_HT_DEBUG) || defined(SENSOR_ENSAHT_DEBUG)
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
// TODO-110 when IO moved to base.cpp; SYSTEM_FS wont need CONTROL

// TODO_ADD_SYSTEM
#if defined(SYSTEM_LOGGER_WANT)
  #define SYSTEM_FS_WANT
#endif
#if defined(SYSTEM_LOGGER_DEBUG)
  #define SYSTEM_TIME_DEBUG
  #define SYSTEM_FS_DEBUG
#endif
#if defined(SYSTEM_SD_WANT) || defined(SYSTEM_SPIFFS_WANT)
  #define SYSTEM_FS_WANT
#endif
#if defined(SYSTEM_SD_DEBUG) || defined(SYSTEM_SPIFFS_DEBUG)
  #define SYSTEM_FS_DEBUG
#endif
#if defined(CONTROL_LOGGERFS_WANT) || defined(CONTROL_GSHEETS_WANT)
  #define CONTROL_LOGGER_WANT 
#endif
#if defined(CONTROL_LOGGERFS_DEBUG) || defined(CONTROL_GSHEETS_DEBUG)
  #define CONTROL_LOGGER_DEBUG 
#endif
#if defined(CONTROL_BLINKEN_WANT) || defined(CONTROL_HYSTERISIS_WANT) || defined(CONTROL_LOGGER_WANT)
  #define CONTROL_WANT
#endif
#if defined(CONTROL_BLINKEN_DEBUG) || defined(CONTROL_HYSTERISIS_DEBUG) || defined(CONTROL_LOGGER_DEBUG) 
  #define CONTROL_DEBUG
#endif
// TO_ADD_SYSTEM - there is no class hierarchy
#if defined(SYSTEM_WIFI_DEBUG) || defined(SYSTEM_MQTT_DEBUG) || defined(SYSTEM_DISCOVERY_DEBUG) || defined(SYSTEM_OTA_DEBUG) || defined(SYSTEM_LORA_DEBUG) || defined(SYSTEM_OLED_DEBUG) || defined(SYSTEM_FS_DEBUG) || defined(SYSTEM_TIME_DEBUG) || defined(SYSTEM_SPI_DEBUG)
  #define SYSTEM_DEBUG
#endif


#if defined(SENSOR_MS5803_SPI) 
  #define SYSTEM_SPI_WANT
#endif
#if defined(SENSOR_MS5803_I2C) || defined(SENSOR_ENSAHT_WANT)
  #define SYSTEM_I2C_WANT
#endif
#if defined(SENSOR_MS5803_DEBUG) 
  #define SYSTEM_SPI_DEBUG
#endif
#if defined(CONTROL_GSHEETS_WANT)
  #define SYSTEM_TIME_WANT
#endif

#if defined(SENSOR_DEBUG) || defined(ACTUATOR_DEBUG) || defined(CONTROL_DEBUG) || defined(SYSTEM_DEBUG)
  #define ANY_DEBUG
#endif 
// TO_ADD_BOARD
// shields compatible with D1 and its ESP8266 not C-pico which has same pin layout but different availability esp of analog
#if defined(ESP8266_D1_MINI_PROv2) || defined (ESP8266_D1_MINI) || defined(ESP8266_D1_PRO_CLONE)
  #define ESP8266_D1
#endif
#if defined(TTGO_LORA_SX1278_V1) || defined(TTGO_LORA_SX1276_V1)
  #define TTGO_LORA_SX127X_V1
#elif defined(TTGO_LORA_SX1278_V21) || defined(TTGO_LORA_SX1276_V21)
  #define TTGO_LORA_SX127X_V21
#elif defined(TTGO_LORA_SX1278_V2) || defined(TTGO_LORA_SX1276_V2)
  #define TTGO_LORA_SX127X_V2
#endif
#if defined(TTGO_LORA_SX1276_V1) || defined(TTGO_LORA_SX1276_V2) || defined(TTGO_LORA_SX1276_V21)
  #define TTGO_LORA_SX1276
#elif defined(TTGO_LORA_SX1278_V1) || defined(TTGO_LORA_SX1278_V2) || defined(TTGO_LORA_SX1276_V21)
  #define TTGO_LORA_SX1278
#endif
#if defined(TTGO_LORA_SX1276) || defined(TTGO_LORA_SX1278)
  #define TTGO_LORA_SX127X
#endif

//TO_ADD_BOARD 
#ifndef SYSTEM_DISCOVERY_DEVICE_DESCRIPTION
  #ifdef ESP8266_D1
    #define BOARDNAME "ESP8266 D1"
  #elif defined(LOLIN_C3_PICO)
    #define BOARDNAME "Lolin C3 Pico"
  #elif defined(LOLIN_S2_MINI)
    #define BOARDNAME "Lolin S2 Mini"
  #elif defined(TTGO_LORA_SX127X)
    #define BOARDNAME "TTGO Lora SX127X" 
  #else
    #error undefined board in system_discovery.cpp #TO_ADD_BOARD
  #endif
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
