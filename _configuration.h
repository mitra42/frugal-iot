/* 
 * This file will be auto-generated, once auto-generation is built, this file shouldn't be human edited
 */

#ifndef FRUGALIOT_CONFIGURATION_H
#define FRUGALIOT_CONFIGURATION_H

// #define WANT_SENSOR_DHT11            // DHT11 Temp and humidity sensor
#define WANT_ACTUATOR_BLINKEN // Demo blinking led on board
#define ACTUATOR_BLINKEN_MS 1000

#define WANT_SENSOR_ANALOG // simple analog read 
#define SENSOR_ANALOG_PIN 3 // Which pin to read - this might be board specific
#define SENSOR_ANALOG_MS 1000 // How often to read in MS

#endif FRUGALIOT_CONFIGURATION_H
