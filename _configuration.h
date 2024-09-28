/* 
 * This file will be auto-generated, once auto-generation is built, this file shouldn't be human edited
 */

#ifndef FRUGALIOT_CONFIGURATION_H
#define FRUGALIOT_CONFIGURATION_H

#define CHIP ESP8266
#define BOARD ESP8266_D1_MINI // Board level - define - will only support boards devs are actually using, but intended to support board specific fixes

#define ACTUATOR_LEDBUILTIN_WANT // Control led on board
//#define ACTUATOR_LEDBUILTIN_DEBUG
#define CONTROL_BLINKEN_WANT // Demo blinking led on board
#define CONTROL_BLINKEN_MS 1000
//#define CONTROL_BLINKEN_DEBUG

#define SENSOR_ANALOG_WANT // simple analog read 
#define SENSOR_ANALOG_PIN A0 // Which pin to read - this might be board specific
#define SENSOR_ANALOG_MS 1000 // How often to read in MS
#define SENSOR_ANALOG_DEBUG
#define SENSOR_ANALOG_SMOOTH 2

#define SENSOR_SHT85_WANT
#define SENSOR_SHT85_DEVICE SHT30 // e.g. The Lolin SHT30 shield
#define SENSOR_SHT85_DEBUG
#define SENSOR_SHT85_MS 1000 // Warning not to do this too often seen range in examples from 100 to 10000 not sure why such range
//#define SENSOR_SHT85_ADDRESS 0x45  // e.g. The Lolin SHT30 shield
#define SENSOR_SHT85_ADDRESS_ARRAY 0x45,0x44  // e.g. The Lolin SHT30 shield
#define SENSOR_SHT85_COUNT 2 // Count of array - makes code easier for fixed length arrays

#define SYSTEM_MQTT_WANT
#define SYSTEM_MQTT_SSID "Zest" // Expect this to move to SYSTEM_WIFI and then be configured in some other way
#define SYSTEM_MQTT_PASSWORD "#zestubud" // Expect this to move to SYSTEM_WIFI and then be configured in some other way
#define SYSTEM_MQTT_SERVER "public.cloud.shiftr.io" // From the demo
#define SENSOR_MQTT_MS 1000 // Message every second
#define SENSOR_MQTT_TOPIC "/hello"
#define SENSOR_MQTT_PAYLOAD "world"
#define SYSTEM_MQTT_DEBUG

#endif FRUGALIOT_CONFIGURATION_H
