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
#include "_configuration.h"      // Include (eventually) auto-generated file, especially defines a lot of "WANT_"

#ifdef WANT_SYSTEM_MQTT
#include "system_mqtt.h"
#endif
#ifdef WANT_ACTUATOR_LEDBUILTIN
#include "actuator_ledbuiltin.h"
#endif
#ifdef WANT_SENSOR_ANALOG
#include "sensor_analog.h"
#endif
#ifdef WANT_SENSOR_SHT85
#include "sensor_sht85.h"
#endif
#ifdef WANT_CONTROL_BLINKEN
#include "control_blinken.h"
#endif


#endif FRUGALIOT_SETTINGS_H
