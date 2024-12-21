/* 
 * This file will be auto-generated, once auto-generation is built, this file shouldn't be human edited
 */

#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H


#define SERIAL_BAUD 460800 // Generally find 460800 works well - reliability on all boards tested
#define SERIAL_DELAY 5000 // Necessary to avoid losing initial messsage in garbage, at least on ESP8266_D1_MINI

//TO_ADD_ACTUATOR - follow the pattern below and add any variables and search for other places tagged TO_ADD_ACTUATOR

#define ACTUATOR_LEDBUILTIN_WANT // Control led on board
#define ACTUATOR_LEDBUILTIN_DEBUG
#define ACTUATOR_LEDBUILTIN_ADVERTISEMENT "\n  -\n    topic: ledbuiltin\n    name: Built in LED\n    type: bool\n    display: toggle\n    rw: rw"

// #define ACTUATOR_RELAY_WANT // Control relay - enable in _local.h
#define ACTUATOR_RELAY_DEBUG
#define ACTUATOR_RELAY_ADVERTISEMENT "\n  -\n    topic: relay\n    name: Relay\n    type: bool\n    display: toggle\n    rw: rw"

// Demo blinking led on board
// #define CONTROL_BLINKEN_WANT // Define in _local.h if want to test with this
#define CONTROL_BLINKEN_S 1
#define CONTROL_BLINKEN_DEBUG
#define CONTROL_BLINKEN_ADVERTISEMENT "\n  -\n    topic: control_blinken_seconds\n    name: Blink period (s)\n    type: int\n    min: 1\n    max: 60\n    display: slider\n    rw: rw"

// Demo controlling LED based on humidity level
//#define CONTROL_DEMO_MQTT_WANT // Define in _local.h if want to test with this
#define CONTROL_DEMO_MQTT_DEBUG
#define CONTROL_DEMO_MQTT_HUMIDITY_MAX 75 // turn on LED above this humidity

//TO_ADD_SENSOR - follow the pattern below and add any variables and search for other places tagged TO_ADD_SENSOR

// simple analog r
//#define SENSOR_ANALOG_WANT // Define in _local.h if using sensor
#define SENSOR_ANALOG_PIN A0 // Which pin to read - this might be board specific
#define SENSOR_ANALOG_MS 10000 // How often to read in MS
#define SENSOR_ANALOG_DEBUG
//#define SENSOR_ANALOG_SMOOTH 2

// #define SENSOR_SHT85_WANT // Define in _local.h if using sensor
#define SENSOR_SHT85_DEVICE SHT30 // e.g. The Lolin SHT30 shield
#define SENSOR_SHT85_DEBUG
#define SENSOR_SHT85_MS 10000 // Warning not to do this too often seen range in examples from 100 to 10000 not sure why such range
#define SENSOR_SHT85_ADDRESS_ARRAY 0x45  // e.g. The Lolin SHT30 shield plus non-existant sensor for testing only
#define SENSOR_SHT85_COUNT 1 // Count of array - makes code easier for fixed length arrays
// #define SENSOR_SHT85_ADDRESS_ARRAY 0x45,0x44  // Example of multiple sensors
// #define SENSOR_SHT85_COUNT 2 // Count of array - makes code easier for fixed length arrays
#define SENSOR_SHT85_TOPIC_TEMPERATURE "temperature"
#define SENSOR_SHT85_TOPIC_HUMIDITY "humidity"
#define SENSOR_SHT85_ADVERTISEMENT "\n  -\n    topic: temperature\n    name: Temperature\n    type: float\n    display: bar\n    min: 0\n    max: 45\n    color: red\n    rw: r\n  -\n    topic: humidity\n    name: Humidity\n    type: float\n    display: bar\n    min: 0\n    max: 100\n    color: cornflowerblue\n    rw: r"

// #define SENSOR_DHT_WANT // Define in _local.h if using sensor
#define SENSOR_DHT_DEVICE DHT11 // as in KY-015 board
#define SENSOR_DHT_DEBUG
#define SENSOR_DHT_MS 10000 // Warning not to do this too often seen range in examples from 100 to 10000 not sure why such range
#define SENSOR_DHT_PIN_ARRAY 4  // hard wired to a pin - 4 is D2 on the Lolin D1 but is somewhere else on C3 Pico
#define SENSOR_DHT_COUNT 1 // Count of array - makes code easier for fixed length arrays
#define SENSOR_DHT_TOPIC_TEMPERATURE "temperature"
#define SENSOR_DHT_TOPIC_HUMIDITY "humidity"
#define SENSOR_DHT_ADVERTISEMENT "\n  -\n    topic: temperature\n    name: Temperature\n    type: float\n    display: bar\n    min: 0\n    max: 45\n    color: red\n    rw: r\n  -\n    topic: humidity\n    name: Humidity\n    type: float\n    display: bar\n    min: 0\n    max: 100\n    color: cornflowerblue\n    rw: r"


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
