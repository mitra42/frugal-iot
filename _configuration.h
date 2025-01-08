/* 
 * This file will be auto-generated, once auto-generation is built, this file shouldn't be human edited
 */

#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H


#define SERIAL_BAUD 460800 // Generally find 460800 works well - reliability on all boards tested
#define SERIAL_DELAY 5000 // Necessary to avoid losing initial messsage in garbage, at least on ESP8266_D1_MINI

//TO_ADD_ACTUATOR - follow the pattern below and add any variables and search for other places tagged TO_ADD_ACTUATOR

// Demo controlling LED based on humidity level
//#define CONTROL_DEMO_MQTT_WANT // Define in _local.h if want to test with this
#define CONTROL_DEMO_MQTT_DEBUG
#define CONTROL_DEMO_MQTT_HUMIDITY_MAX 75 // turn on LED above this humidity

#define SYSTEM_WIFI_WANT // Will always want WiFi until have BLE &/or LoRa
#define SYSTEM_WIFI_DEBUG
#define SYSTEM_WIFI_PORTAL_RESTART 120000 // How long (ms) portal should wait before restarting - 2 mins probably about right

#define SYSTEM_MQTT_WANT // At this point, commenting this line out is not going to compile - too many dependencies on MQTT
#define SYSTEM_MQTT_MS 10 //Run client every 10ms
#define SYSTEM_MQTT_DEBUG
#define SYSTEM_MQTT_LOOPBACK // If true dispatch the message locally as well.

#define SYSTEM_DISCOVERY_WANT // Almost always set, will tell the MQTT server about the device so the Client can find it. 
#define SYSTEM_DISCOVERY_DEBUG
#define SYSTEM_DISCOVERY_MS 30000

#endif // _CONFIGURATION_H
