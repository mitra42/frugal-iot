#ifndef LOCAL_H
#define LOCAL_H
/* 
 * Copy this file to _local.h and edit it
 * Use it for local #ifdefs
 * Its the place to list the configuration of your device e.g. defines like SENSOR_SHT_WANT 
 *
 * _local-template.h will be on github, but _local.h will not - so its a safer place for passwords etc.
 */

// Organization behind this set of devices. Also used for MQTT userid
#ifdef KOPERNIK1
  #define SYSTEM_DISCOVERY_ORGANIZATION "kopernik"
  #define SYSTEM_MQTT_USER SYSTEM_DISCOVERY_ORGANIZATION  // Edit if you use something other than the organization name for MQTT user
  // Specify the password for your organization on the MQTT server
  #define SYSTEM_MQTT_PASSWORD "findingwhatworks"
#else
  #define SYSTEM_DISCOVERY_ORGANIZATION "dev"
  #define SYSTEM_MQTT_USER SYSTEM_DISCOVERY_ORGANIZATION  // Edit if you use something other than the organization name for MQTT user
  // Specify the password for your organization on the MQTT server
  #define SYSTEM_MQTT_PASSWORD "public"
#endif
#define SYSTEM_MQTT_SERVER "frugaliot.naturalinnovation.org"

// It can be slow developing by resting the portal each time, so if wanted define SSID and PASSWORD here, 
// just don't forget to comment these out before release
//#define SYSTEM_WIFI_SSID "myhomewifi"
//#define SYSTEM_WIFI_PASSWORD "somesecretpassword"
#define SYSTEM_WIFI_PROJECT "lotus"
// If you define SSID and PASSWORD it will always connect to this - overriding whatever is set in the portal - this is NOT what you usually want.
// generally 
  // SSID_1 through SSID_9 define WiFi networks it will try if they show up in the scan, 
  // it will also remember any it successfully connects to after entering in the portal (Tools->Erase All Flash in arduino IDE to delete)
  #define SYSTEM_WIFI_SSID_1 "myhomewifi"
  #define SYSTEM_WIFI_PASSWORD_1 "blahblah"
  #define SYSTEM_WIFI_SSID_2 "cafe"
  #define SYSTEM_WIFI_PASSWORD_2 "12345678"

// How often to check for software updates - 300000 is good for dev, comment out to get default of an hour
#define SYSTEM_OTA_MS 300000 // 5 minute updates

//Since I work on multiple nodes, I define each here, then uncomment exactly one and have its settings below. 
#define BOARD1 "ESP8266 D1 mini with SHT sensor"
//#define BOARD2 "ESP32 C3 Pico with Soil and Battery sensor"
//#define SONOFF_R2 "Sonoff R2"
//#define LILYGOHIGROW1 "Lilygo HiGrow"
//#define BOARDCOMPILECHECK "Check all code compiles"

// Note that on ESP that ESP32 or ESP8266 will be defined - should define other chips names here if its not ESP32 or ESP8266
#ifdef BOARD1
  #ifndef ESP8266
    #error should be using Lolin(Wemos) D1 Pico in the IDE, and looks like you are using a ESP32 board
  #endif
  #define ESP8266_D1_MINI // Board level - define - will only support boards devs are actually using, but intended to support board specific fixes
  #define SYSTEM_WIFI_DEVICE "ESP8266-SHT" // This is default for a short name that appears in the UX. e.g. could be "main pump" - wifi portal can override
  #define SYSTEM_DISCOVERY_DEVICE_DESCRIPTION BOARD1 // override automatically generated description for this device 
  #define ACTUATOR_LEDBUILTIN_WANT // LED on board - usually wanted
  #define SENSOR_SHT_WANT
  #define SENSOR_SHT_MS 10000 // for development only
  #define SYSTEM_OTA_WANT
  #define SYSTEM_OTA_KEY "esp8266-sht"
#endif //BOARD1

#ifdef BOARD2
  #ifndef ESP32
    #error Board 3 should be Lolin C3 Pico as board
  #endif
  #define LOLIN_C3_PICO
  #define SYSTEM_WIFI_DEVICE "ESP32Cpico"
  #define SYSTEM_DISCOVERY_DEVICE_DESCRIPTION BOARD3 // override automatically generated description for this device
  #define ACTUATOR_LEDBUILTIN_WANT // LED on board - usually wanted
  #define SENSOR_SOIL_WANT
  #define SENSOR_SOIL_MS 10000
  #define SENSOR_SOIL_DEBUG
  #define SENSOR_BATTERY_WANT
  #define SENSOR_BATTERY_DEBUG
  #define SENSOR_BATTERY_MS 15000 // While debugging I want readings every 15 seconds
  #define SYSTEM_OTA_WANT
  #define SYSTEM_OTA_KEY "esp32-soil"
#endif //BOARD2


#ifdef SONOFF_R2
  // See https://github.com/mitra42/frugal-iot/issues/108
  #ifndef ESP8266
    #error should be using ITEAD_SONOFF in the IDE which defines ESP8266 even though the Sonoff R2 is ESP8285
  #endif
  #define ITEAD_SONOFF
  #define SYSTEM_DISCOVERY_DEVICE_DESCRIPTION SONOFF_R2
  #define ACTUATOR_RELAY_WANT
  #define SYSTEM_OTA_WANT
  #define SYSTEM_OTA_KEY "sonoff-r2"
  #define SYSTEM_WIFI_PROJECT "lotus"
#endif

#ifdef LILYGOHIGROW1
  #ifndef ESP32
    #error should be using "ESP32 Dev Module" in the IDE, and looks like you are using a ESP8266 board
  #endif
  #define LILYGOHIGROW // For board determined decisions of things like pins
  #define SYSTEM_WIFI_DEVICE "LilyGo HiGrow"
  #define SYSTEM_DISCOVERY_DEVICE_DESCRIPTION LILYGOHIGROW1
  // Below here need developing 
//#define ACTUATOR_LEDBUILTIN_WANT // doesnt appear to be a LED
  #define SENSOR_DHT_WANT
  //#define SENSOR_LIGHT_WANT
  #define SENSOR_SOIL_WANT
  #define SENSOR_SOIL_MS 10000 // For debugging
  #define SENSOR_SOIL_DEBUG
  //#define SENSOR_ANALOG_INSTANCES_WANT
  #define SENSOR_ANALOG_INSTANCES_DEBUG
  #define SENSOR_ANALOG_PIN_1 34
  #define SENSOR_ANALOG_TOPIC_1 "salt"
  #define SENSOR_ANALOG_NAME_1 "Salt"
  #define SENSOR_ANALOG_MS_1 2000 // For debugging
  #define SENSOR_ANALOG_SMOOTH_1 4
  //#define SENSOR_BATTERY_WANT // Untested as battery never arrived ! 
  #define SENSOR_BH1750_WANT
  #define SENSOR_BH1750_DEBUG
  //#define ACTUATOR_RELAY
  //#define SENSOR_BATTERY
  #define SENSOR_BUTTON_WANT
  #define POWER_CTRL 4 // Required before reading anything TODO-power consumption turnoff outside loops TODO-115 move this to a system_power module
  #define SYSTEM_WIFI_PROJECT "lotus"
#endif

#ifdef BOARDCOMPILECHECK // FOr checking everything compiles - check with both ESP32 and ESP8266 boards
  #define SYSTEM_WIFI_PROJECT "lotus"
  //TO_ADD_SENSOR
  #define SENSOR_DHT_WANT
  #define SENSOR_SHT_WANT
  #define SENSOR_SOIL_WANT
  #define SENSOR_SOIL_PIN 4
  #define SENSOR_ANALOG_INSTANCES_WANT
  #define SENSOR_ANALOG_PIN_1 4
  #define SENSOR_BATTERY_WANT // Only works currently on ESP8266
  #define SENSOR_BH1750_WANT
  #define SENSOR_BH1750_ADDRESS 0x23
  //TO_ADD_ACTUATOR
  #define ACTUATOR_LEDBUILTIN_WANT // LED on board - usually wanted
  #define ACTUATOR_RELAY_WANT
  //TO_ADD_CONTROL
  #define CONTROL_BLINKEN_WANT
  #define CONTROL_HYSTERISIS_WANT
  #ifdef ESP8266
    #define ESP8266_D1_MINI
    #define SYSTEM_OTA_WANT // Only works currently on ESP8266
    #define SYSTEM_OTA_KEY "xxx" // Dummy - dont want it to OTA
  #endif
  #ifdef ESP32
    #define LOLIN_C3_PICO
    #define SENSOR_SOIL_WANT
    #define SYSTEM_TIME_WANT
  #endif
  //#define SYSTEM_OTA_KEY - no OTA key - dont want anything picking this code up! 
#endif

// Can also define a default language
// #define LANGUAGE_DEFAULT "de"
// Want debugging usually onfollowing
#define SYSTEM_DISCOVERY_DEBUG
#define SYSTEM_WIFI_DEBUG
#define SYSTEM_MQTT_DEBUG

#endif //LOCAL_H