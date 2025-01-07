/* 
 * Copy this file to _local.h and edit it
 * Use it for local #ifdefs, for example that override global ones in _configuration.h 
 * Its the place to list the configuration of your device e.g. defines like SENSOR_SHT_WANT 
 *
 * _local-template.h will be on github, but _local.h will not - so its a safer place for passwords etc.
 */

// Organization behind this set of devices. Also used for MQTT userid
#define SYSTEM_DISCOVERY_ORGANIZATION "dev"
#define SYSTEM_MQTT_USER SYSTEM_DISCOVERY_ORGANIZATION  // Edit if you use something other than the organization name for MQTT user
// Specify the password for your organization on the MQTT server
#define SYSTEM_MQTT_PASSWORD "public"
// Where your MQTT server is - you can use naturalinnovation.org for testing, but talk to us if you are deploying live sites using it.
#define SYSTEM_MQTT_SERVER "naturalinnovation.org"

// It can be slow developing by resting the portal each time, so if wanted define SSID and PASSWORD here, 
// just don't forget to comment these out before release
//#define SYSTEM_WIFI_SSID "myhomewifi"
//#define SYSTEM_WIFI_PASSWORD "somesecretpassword"
#define SYSTEM_WIFI_PROJECT "lotus"

//Since I work on multiple nodes, I define each here, then uncomment exactly one and have its settings below. 
#define BOARD1 "ESP8266 D1 mini with SHT sensor"
// #define BOARD2 "ESP8266 D1 mini with DHT sensor"
//#define BOARD3 "ESP32 C3 Pico with Soil and Battery sensor"

// Note that on ESP that ESP32 or ESP8266 will be defined - should define other chips names here if its not ESP32 or ESP8266
#ifdef BOARD1
  #ifndef ESP8266
    #error should be using Lolin(Wemos) D1 Pico in the IDE, and looks like you are using a ESP32 board
  #endif
  #define ESP8266_D1_MINI // Board level - define - will only support boards devs are actually using, but intended to support board specific fixes
  #define SYSTEM_WIFI_DEVICE "ESP8266-SHT" // SHT
  #define SYSTEM_DISCOVERY_DEVICE_DESCRIPTION BOARD1 // override automatically generated description for this device 
  #define SENSOR_SHT_WANT
  #define SENSOR_SHT_MS 10000 // for development only
#endif //BOARD1
#ifdef BOARD2
  #ifndef ESP8266
    #error should be using Lolin(Wemos) D1 Pico in the IDE, and looks like you are using a ESP32 board
  #endif
  #define SYSTEM_WIFI_DEVICE "ESP8266-DHT"
  #define ESP8266_D1_MINI // Board level - define - will only support boards devs are actually using, but intended to support board specific fixes
  #define SYSTEM_DISCOVERY_DEVICE_DESCRIPTION BOARD2 // override automatically generated description for this device
  #define SENSOR_DHT_WANT
  #define SENSOR_DHT_MS 10000 // For debugging
#endif //BOARD2
#ifdef BOARD3
  #ifndef ESP32
    #error should be Lolin C3 Pico as board
  #endif
  #define LOLIN_C3_PICO
  #define SYSTEM_WIFI_DEVICE "ESP32Cpico"
  #define SYSTEM_DISCOVERY_DEVICE_DESCRIPTION BOARD3 // override automatically generated description for this device
  #define SENSOR_SOIL_WANT
  #define SENSOR_SOIL_MS 10000 // While debugging I want readings every 10 seconds
  #define SENSOR_SOIL_DEBUG
  #define SENSOR_BATTERY_WANT
  #define SENSOR_BATTERY_DEBUG
  #define SENSOR_BATTERY_MS 15000 // While debugging I want readings every 15 seconds
#endif //BOARD3

// Wanted on all boards
#define ACTUATOR_LEDBUILTIN_WANT

// Optional devices that work on multiple boards
//#define ACTUATOR_RELAY_WANT
//#define SENSOR_ANALOG_EXAMPLE_WANT

// Non physical devices e.g. controls
//#define CONTROL_BLINKEN_WANT
//#define CONTROL_DEMO_MQTT_WANT

// OTA is experimental - only works on ESP8266 and not yet ready for release
#ifdef ESP8266_D1_MINI
  // #define SYSTEM_OTA_WANT
#endif

// Can also define a default language
// #define LANGUAGE_DEFAULT "de"
