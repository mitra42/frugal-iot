/*
 * This is a place to configure all aspects of the compilation
 * Can add sensor-specific settings here, also those needed to control imported libraries
 * Note common structures etc go in _common.ini
 * This file will be built automatically from something in YAML
 * OR there will be part auto-built and that will be included from here
 */
#ifndef FRUGALIOT_SETTINGS_H
#define FRUGALIOT_SETTINGS_H

#define FRUGALIOT_DEBUG       // Turn on top level debugging - TODO if this is off may get errors in Serial.println
#include "_configuration.h"      // Include (eventually) auto-generated file, defines _WANT and parameters for each module"

#endif FRUGALIOT_SETTINGS_H
