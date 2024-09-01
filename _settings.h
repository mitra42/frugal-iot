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

// #define WANT_SENSOR_DHT11            // DHT11 Temp and humidity sensor
#define WANT_ACTUATOR_BLINKEN

#endif FRUGALIOT_SETTINGS_H
