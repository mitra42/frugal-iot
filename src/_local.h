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

//Since I work on multiple nodes, I define each here, then uncomment exactly one and have its settings below. 
#ifndef PLATFORMIO // On PlatformIO use the platformio.ini to choose project(s)
#define BOARDCOMPILECHECK "Pseudo board to test everything compiles".
//#define BOARDDEV "Working short term dev"
//#define ISSUE25 "Control class"
//#define DATALOGGER110 "Data logger"
//#define SONOFF_R2 "Sonoff R2"
//#define BOARDMAGI1 "ESP8266 D1 mini pro v2 green with SHT"
//#define BOARDMAGI2 "ESP32 C3 pico with SHT and Soil"
//#define BOARDMAGI3 "ESP8266 D1 pro clone battery and antenna with SHT"
//#define BOARDMAGI4 "ESP32 C3 pico with SHT"
//#define BOARDMAGI5 "ESP8266 D1 pro green + SHT -2"
//#define BOARDMAGI6 "ESP8266 D1 pro blue + SHT"
//#define LILYGOHIGROW1 "Lilygo HiGrow dev unit"
//#define BOARDDEMO1 "ESP8266 D1 pro green + SHT -1"
//#define BOARDDEMO2 "ESP8266 D1 pro blue + SHT + Relay"
//#define KOPERNIK1 "Kopernik pump control"
//#define ISSUE112
#endif

// Define which board you are using - or it will use a default
#ifndef PLATFORMIO // If you are using PlatformIO, it will define the board for you
// #define ARDUINO_LOLIN_C3_PICO
#endif //PLATFORMIO


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
// If you define SSID and PASSWORD it will always connect to this - overriding whatever is set in the portal - this is NOT what you usually want.
// generally 
  // SSID_1 through SSID_9 define WiFi networks it will try if they show up in the scan, 
  // it will also remember any it successfully connects to after entering in the portal (Tools->Erase All Flash in arduino IDE to delete)
#if defined(BOARDMAGI5) || defined(BOARDMAGI2) || defined(BOARDMAGI1) || defined(BOARDMAGI6)
  #define SYSTEM_WIFI_SSID_1 "Ima"
  #define SYSTEM_WIFI_PASSWORD_1 "Feb346529"
  #define SYSTEM_WIFI_SSID_2 "Ryoko_30CFC3" // Muama device
  #define SYSTEM_WIFI_PASSWORD_2 "12345678"
#elif defined(KOPERNIK1)
  #define SYSTEM_WIFI_SSID_1 "KOPERNIK"
  #define SYSTEM_WIFI_PASSWORD_1 "findingwhatworks"
#elif defined(BALIWIFI)
  #define SYSTEM_WIFI_SSID_3 "La Luz 1"
  #define SYSTEM_WIFI_PASSWORD_3 "somosluz"
  #define SYSTEM_WIFI_SSID_4 "Garden Kafe"
  #define SYSTEM_WIFI_PASSWORD_4 "gardenonline"
  #define SYSTEM_WIFI_SSID_5 "GEREBIG VILLA 2"
  #define SYSTEM_WIFI_PASSWORD_5 "KADEKGRB"
  #define SYSTEM_WIFI_SSID_6 "GREEN RABBIT"
  #define SYSTEM_WIFI_PASSWORD_6 "FODLAB2020"
  #define SYSTEM_WIFI_SSID_7 "Zest"
  #define SYSTEM_WIFI_PASSWORD_7 "#zestubud"
  #define SYSTEM_WIFI_SSID_8 "GEREBIG VILLA 1"
  #define SYSTEM_WIFI_PASSWORD_8 "GRB575800"
#else
  #define SYSTEM_WIFI_SSID_1 "EXETEL 8BFC34 2.4G"
  #define SYSTEM_WIFI_PASSWORD_1 "urGGFHQN"
  #define SYSTEM_WIFI_SSID_2 "EXETEL 8BFC34 5G"
  #define SYSTEM_WIFI_PASSWORD_2 "urGGFHQN"
  #define SYSTEM_WIFI_SSID_3 "LOVEStARS"
  #define SYSTEM_WIFI_PASSWORD_3 "lotusponds1"
  #define SYSTEM_WIFI_SSID_4 "WiFi-A634E9" // Paseyo
  #define SYSTEM_WIFI_PASSWORD_4 "64346298"
#endif
  // The rest of this list are included for development purposes - its where I code ! 
  #define SYSTEM_WIFI_SSID_9 "Silver"
  #define SYSTEM_WIFI_PASSWORD_9 "O2IL-w8k2-w6ki-TzqG"

// Note that on ESP that ESP32 or ESP8266 will be defined - should define other chips names here if its not ESP32 or ESP8266
#ifdef BOARDCOMPILECHECK // FOr checking everything compiles - check with both ESP32 and ESP8266 boards
  #define SYSTEM_WIFI_PROJECT "lotus"
  //TO_ADD_SENSOR
  #define SENSOR_ANALOG_INSTANCES_WANT
  #define SENSOR_ANALOG_PIN_1 4
  #define SENSOR_BATTERY_WANT // Only works currently on ESP8266
  #define SENSOR_BH1750_WANT
  #define SENSOR_BH1750_ADDRESS 0x23
  #define SENSOR_BUTTON_WANT
  #define SENSOR_BUTTON_PIN 4
  #define SENSOR_DHT_WANT
  #define SENSOR_ENSAHT_WANT
  #define SENSOR_LOADCELL_WANT
  #define SENSOR_MS5803_WANT
  #define SENSOR_MS5803_I2C 0x77
  #define SENSOR_SHT_WANT
  #define SENSOR_SOIL_WANT
  #define SENSOR_SOIL_PIN 4
  //TO_ADD_ACTUATOR
  #define ACTUATOR_LEDBUILTIN_WANT // LED on board - usually wanted
  #define ACTUATOR_RELAY_WANT
  #define ACTUATOR_RELAY_PIN 22 // Dummy pin for testing
  //TO_ADD_CONTROL
  #define CONTROL_BLINKEN_WANT
  #define CONTROL_HYSTERISIS_WANT
  #define CONTROL_GSHEETS_WANT
  #define CONTROL_GSHEETS_URL "https://blahblah"
  #define CONTROL_LOGGERFS_WANT
  #define SYSTEM_SD_WANT
  #define SYSTEM_SPIFFS_WANT // ALso includes LittleFS
  #if defined(ESP8266) && !defined(PLATFORMIO)
   // #define ARDUINO_ESP8266_WEMOS_D1MINIPRO  // Use Mini Pro for testing because (V2) supports battery 
  #endif
  //Not sure why we are doing this 
  //#if defined(ESP32) && !defined(PLATFORMIO)
  //    #define ARDUINO_LOLIN_C3_PICO
  //#endif
  #ifdef ESP32 // Functions that only work on ESP32
    #define SYSTEM_TIME_WANT
  #endif
  #define SYSTEM_POWER_MODE_LOOP
  //Need to use ARDUINO_LOLIN_C3_PICO for many of the sensors, so cant test LoRa or OLED here yet
  //#define SYSTEM_LORA_WANT
  //#define SYSTEM_LORAMESHER_WANT
  //#define SYSTEM_LORAMESHER_SENDER_TEST // Remove when LORAMESHER via MQTT built
  //#define SYSTEM_OLED_WANT
  #define SYSTEM_OTA_WANT 
  #define SYSTEM_OTA_KEY "xxx" // Dummy - dont want it to OTA
#endif

#ifdef DATALOGGER110 // testing Logger
  #define SYSTEM_WIFI_DEVICE "Dev 110"
  #define SYSTEM_TIME_WANT
  #define SYSTEM_TIME_DEBUG
  #define CONTROL_LOGGERFS_WANT
  #define SYSTEM_SD_WANT
  #define SYSTEM_SPIFFS_WANT // ALso includes LittleFS
  #define SYSTEM_SD_DEBUG
  #define SYSTEM_SPIFFS_DEBUG
  #define CONTROL_LOGGERFS_DEBUG
  #define CONTROL_WANT
  #define CONTROL_DEBUG
  #define SYSTEM_WIFI_PROJECT "lotus"
  #define ACTUATOR_LEDBUILTIN_WANT
  #define SENSOR_SHT_WANT
  #define SENSOR_SHT_DEBUG
  #define SYSTEM_POWER_MODE_LOOP
#endif

#ifdef ISSUE25 // Stripped down for quick compilations
  #define SYSTEM_WIFI_DEVICE "Dev 25"
  //#define ACTUATOR_LEDBUILTIN_WANT
  //#define CONTROL_HYSTERISIS_WANT
  //#define CONTROL_HYSTERISIS_DEBUG
  #define SYSTEM_WIFI_PROJECT "lotus"
#endif

#ifdef BOARDDEV // Stripped down for quick compilations
  #define SYSTEM_WIFI_DEVICE "Quick dev"
  #define SYSTEM_WIFI_PROJECT "lotus"
  #define SENSOR_ANALOG_INSTANCES_WANT
#endif

#ifdef SONOFF_R2
  // See https://github.com/mitra42/frugal-iot/issues/108
  #ifndef ESP8266
    #error should be using ITEAD_SONOFF in the IDE which defines ESP8266 even though the Sonoff R2 is ESP8285
  #endif
  #define ITEAD_SONOFF
  #define SYSTEM_DISCOVERY_DEVICE_DESCRIPTION SONOFF_R2
  #define ACTUATOR_RELAY_WANT
  #define SYSTEM_WIFI_PROJECT "lotus"
  #define ACTUATOR_LED_WANT
  #define SYSTEM_WIFI_DEVICE "Sonoff switch"
  #define CONTROL_HYSTERISIS_WANT
  #define SYSTEM_OTA_WANT
  #define SYSTEM_OTA_KEY "sonoff"
#endif



#ifdef LILYGOHIGROW1
  #ifndef ESP32
    #error should be using "ESP32 Dev Module" in the IDE, and looks like you are using a ESP8266 board
  #endif
  #define LILYGOHIGROW // For board determined decisions of things like pins
  #define SYSTEM_WIFI_DEVICE "LilyGo HiGrow"
  #define SYSTEM_DISCOVERY_DEVICE_DESCRIPTION LILYGOHIGROW1
  // Below here need developing 
  #define ACTUATOR_LEDBUILTIN_WANT // unclear if there is a LED
  #define SENSOR_DHT_WANT
  //#define SENSOR_LIGHT_WANT
  #define SENSOR_SOIL_WANT
  #define SENSOR_SOIL_DEBUG
  //#define SENSOR_ANALOG_INSTANCES_WANT
  #define SENSOR_ANALOG_INSTANCES_DEBUG
  #define SENSOR_ANALOG_PIN_1 34
  #define SENSOR_ANALOG_TOPIC_1 "salt"
  #define SENSOR_ANALOG_NAME_1 "Salt"
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

// Can also define a default language
#define LANGUAGE_DEFAULT "id"

#ifdef KOPERNIK1
  #ifndef ESP32
    #error should be using "ESP32 WROVER MODULE" in the IDE, and looks like you are using a ESP32 board
  #endif
  #define ESP32_WROVER_DEV
  #define SYSTEM_WIFI_DEVICE "Kopernik pump control"
  #define SYSTEM_DISCOVERY_DEVICE_DESCRIPTION KOPERNIK1
  //#define ACTUATOR_LEDBUILTIN_WANT // LED on board - usually wanted
  #define ACTUATOR_RELAY_WANT
  #define ACTUATOR_RELAY_PIN 4
  #define SENSOR_SOIL_WANT
  #define SENSOR_SOIL_PIN 36
  #define SENSOR_ANALOG_ATTENUATION ADC_11db
  #define SENSOR_SOIL_DEBUG
  #define SENSOR_SOIL_0 3000
  #define SENSOR_SOIL_100 800
  //#define SENSOR_SHT_WANT
  #define CONTROL_HYSTERISIS_WANT
  //#define SYSTEM_OTA_WANT
  #define SYSTEM_OTA_KEY "kopernik1"
  #define SYSTEM_WIFI_PROJECT "lotus"
  //#define LOCAL_DEV_WANT // Include custom code
#endif //BOARDEMO1

#ifdef ISSUE112
  #define SENSOR_DHT_WANT
  #define SYSTEM_WIFI_PROJECT "lotus"
  #define SENSOR_DHT_DEBUG
#endif

#ifdef ISSUE132MS5803
  #define SENSOR_MS5803_WANT
  #define SENSOR_MS5803_DEBUG
  //#define SENSOR_MS5803_SPI 15 // CS Pin for SPI on D1-mini is D8=15 probably different on ESP32
  #define SENSOR_MS5803_I2C 0x77 // Set via jumper to 76 or 77 
#endif

#ifndef SYSTEM_WIFI_PROJECT
  #define SYSTEM_WIFI_PROJECT "lotus"
#endif

#endif //LOCAL_H