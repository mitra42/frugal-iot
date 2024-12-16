/* 
 * Copy this file to _local.h and edit it
 * Use it for local #ifdefs, for example that override global ones in _configuration.h 
 * Its also a great place to list the configuration of your device e.g. defines like SENSOR_SHT85_WANT 
 *
 * _local-template.h will be on github, but _local.h will not - so its a safer place for passwords etc.
 */

// Processor definitions - to allow for code that works across different boards
// Note that on ESP that ESP32 or ESP8266 will be defined - should define other chips names here if its not ESP32 or ESP8266

// Board and processor definitions - to allow for code that works across different boards #TO_ADD_NEW_BOARD 
// Should always uncomment exactly one of these - or define your own board
#define ESP8266_D1_MINI // Board level - define - will only support boards devs are actually using, but intended to support board specific fixes
//#define LOLIN_C3_PICO // For some bug workaround in actuator_ledbuiltin.cpp

// Organization behind this set of devices. Also used for MQTT userid //TODO-51 setup process to register new ones
#define SYSTEM_DISCOVERY_ORGANIZATION "dev"
#define SYSTEM_MQTT_USER SYSTEM_DISCOVERY_ORGANIZATION  // Edit if you use something other than the organization name for MQTT user
// Specify the password for your organization on the MQTT server
#define SYSTEM_MQTT_PASSWORD "public"
// Where your MQTT server is - if using naturalinnovation.org for more than some development please let us know #TODO say how ! 
#define SYSTEM_MQTT_SERVER "naturalinnovation.org"

// It can be slow developing by resting the portal each time, so if wanted define SSID and PASSWORD here, 
// just don't forget to comment these out before release
//#define SYSTEM_WIFI_SSID "myhomewifi"
//#define SYSTEM_WIFI_PASSWORD "somesecretpassword"
//#define SYSTEM_WIFI_DEVICE "ESP8266-A" // SH%

// Can override automatically generated description for your device e.g.
//#define SYSTEM_DISCOVERY_DEVICE_DESCRIPTION "ESP8266 with SHT85 sensor" 

// Now define the actual (non-default) sensors and actuators used and any non-default settings
#define ACTUATOR_LEDBUILTIN_WANT
//#define ACTUATOR_RELAY_WANT
//#define CONTROL_BLINKEN_WANT
//#define CONTROL_DEMO_MQTT_WANT
//#define SENSOR_ANALOG_WANT
//#define SENSOR_BATTERY_WANT
//#define SENSOR_BATTERY_MS 3000 // testing at 3secs while developing
//#define SENSOR_BATTERY_DEBUG
//#define SENSOR_SHT85_WANT
//#define SENSOR_DHT_WANT

// Can also define a default language
// #define LANGUAGE_DEFAULT "de"

