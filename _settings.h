/*
 * This is a place to configure all aspects of the compilation
 * Can add sensor-specific settings here, also those needed to control imported libraries
 * Note common structures etc go in _common.ini
 * This file will be built automatically from something in YAML
 * OR there will be part auto-built and that will be included from here
 */
#ifndef FRUGALIOT_SETTINGS_H
#define FRUGALIOT_SETTINGS_H

#define FRUGALIOT_DEBUG       // Turn on top level debugging
#include "_configuration.h"      // Include (eventually) auto-generated file, defines _WANT and parameters for each module"

#ifdef SYSTEM_MQTT_WANT
#include "system_mqtt.h"
#endif
#ifdef ACTUATOR_LEDBUILTIN_WANT
#include "actuator_ledbuiltin.h"
#endif
#ifdef SENSOR_ANALOG_WANT
#include "sensor_analog.h"
#endif
#ifdef SENSOR_SHT85_WANT
#include "sensor_sht85.h"
#endif
#ifdef CONTROL_BLINKEN_WANT
#include "control_blinken.h"
#endif


#endif FRUGALIOT_SETTINGS_H
