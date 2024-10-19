/* 
 * This file will be auto-generated, once auto-generation is built, this file shouldn't be human edited
 */

#ifndef FRUGALIOT_CONFIGURATION_H
#define FRUGALIOT_CONFIGURATION_H

// Board and processor definitions - to allow for code that works across different boards
// Note that on ESP that ESP32 or ESP8266 will be defined - should define other chips names here.
#define ESP8266_D1_MINI // Board level - define - will only support boards devs are actually using, but intended to support board specific fixes
//#define LOLIN_C3_PICO // For some bug workaround in actuator_ledbuiltin.cpp

#define SERIAL_BAUD 460800
#define SERIAL_DELAY 5000 // Necessary to avoid losing initial messsage in garbage, at least on ESP8266_D1_MINI

#define ACTUATOR_LEDBUILTIN_WANT // Control led on board
#define ACTUATOR_LEDBUILTIN_DEBUG
#define ACTUATOR_LEDBUILTIN_TOPIC "/ledbuiltin"

// Demo blinking led on board
#define CONTROL_BLINKEN_WANT 
#define CONTROL_BLINKEN_MS 1000
//#define CONTROL_BLINKEN_DEBUG

// Demo controlling LED based on humidity level
#define CONTROL_DEMO_MQTT_WANT
#define CONTROL_DEMO_MQTT_DEBUG
#define CONTROL_DEMO_MQTT_HUMIDITY_MAX 75 // turn on LED above this humidity


//#define SENSOR_ANALOG_WANT // simple analog read 
#define SENSOR_ANALOG_PIN A0 // Which pin to read - this might be board specific
#define SENSOR_ANALOG_MS 1000 // How often to read in MS
#define SENSOR_ANALOG_DEBUG
#define SENSOR_ANALOG_SMOOTH 2

#define SENSOR_SHT85_WANT
#define SENSOR_SHT85_DEVICE SHT30 // e.g. The Lolin SHT30 shield
#define SENSOR_SHT85_DEBUG
#define SENSOR_SHT85_MS 1000 // Warning not to do this too often seen range in examples from 100 to 10000 not sure why such range
#define SENSOR_SHT85_ADDRESS_ARRAY 0x45  // e.g. The Lolin SHT30 shield plus non-existant sensor for testing only
#define SENSOR_SHT85_COUNT 1 // Count of array - makes code easier for fixed length arrays
// #define SENSOR_SHT85_ADDRESS_ARRAY 0x45,0x44  // Example of multiple sensors
// #define SENSOR_SHT85_COUNT 2 // Count of array - makes code easier for fixed length arrays
#define SENSOR_SHT85_TOPIC_TEMPERATURE "/temperature"
#define SENSOR_SHT85_TOPIC_HUMIDITY "/humidity"

#define SYSTEM_WIFI_WANT
#define SYSTEM_WIFI_DEBUG
#define SYSTEM_MQTT_WANT // At this point, commenting this line out is not going to complie - too many dependencies on MQTT
#define SYSTEM_MQTT_SERVER "public.cloud.shiftr.io" // From the demo - default - can be overridden
#define SYSTEM_MQTT_MS 10 //Run client every 10ms
#define SYSTEM_MQTT_DEBUG
#define SYSTEM_MQTT_LOOPBACK // If true dispatch the message locally as well.

#endif FRUGALIOT_CONFIGURATION_H
