/*
 * This is a place to configure all aspects of the compilation
 * Can add sensor-specific settings here, also those needed to control imported libraries
 * Note common structures etc go in _common.ini
 * This file will be built automatically from something in YAML
 * OR there will be part auto-built and that will be included from here
 */
#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <Arduino.h> // make sure CONFIG_IDF_TARGET_ESP32C3 etc defined if on those boards

#if !defined(PLATFORMIO)
  // On Arduiono do not have platformio.ini so presume these standards
  // TO-ADD-SENSOR TO-ADD-CONTROL TO-ADD-ACTUATOR TO-ADD-SYSTEm
  #define CONTROL_BLINKEN_DEBUG
  #define CONTROL_LOGGERFS_DEBUG
  #define SENSOR_ENSAHT_DEBUG
  #define SENSOR_LOADCELL_DEBUG
  #define SENSOR_MS5803_DEBUG
  #define SENSOR_SHT_DEBUG
  #define SYSTEM_DISCOVERY_DEBUG
  #define SYSTEM_FRUGAL_DEBUG
  #define SYSTEM_LITTLEFS_DEBUG
  #define SYSTEM_MQTT_DEBUG
  #define SYSTEM_TIME_DEBUG
  #define SYSTEM_POWER_DEBUG
  #define SYSTEM_WIFI_DEBUG
#endif

#define MQTT_RETAIN true
#define MQTT_DONT_RETAIN false
#define MQTT_QOS_NONE 0
#define MQTT_QOS_ATLEAST1 1
#define MQTT_QOS_EXACTLY1 2

// TO_ADD_SENSOR - add in appropriate line below depending on superclass
#if defined(SENSOR_SHT_DEBUG) || defined(SENSOR_DHT_DEBUG)
  #define SENSOR_HT_DEBUG
#endif
#if defined(SENSOR_BATTERY_DEBUG) || defined(SENSOR_SOIL_DEBUG)
  #define SENSOR_ANALOG_DEBUG
#endif
#if defined(SENSOR_ANALOG_DEBUG) 
  #define SENSOR_UINT16_DEBUG
#endif
#if defined(SENSOR_BH1750_DEBUG) || defined(SENSOR_LOADCELL_DEBUG)
  #define SENSOR_FLOAT_DEBUG
#endif
#if defined(SENSOR_UINT16_DEBUG) || defined(SENSOR_FLOAT_DEBUG) || defined(SENSOR_HT_DEBUG) || defined(SENSOR_ENSAHT_DEBUG)
  #define SENSOR_DEBUG // Only used for ANY_DEBUG below
#endif



//  TO_ADD_ACTUATOR - add in appropriate line below depending on superclass
#if defined(ACTUATOR_RELAY_DEBUG) || defined(ACTUATOR_LEDBUILTIN_DEBUG)
  #define ACTUATOR_DIGITAL_DEBUG
#endif
#if defined(ACTUATOR_DIGITAL_DEBUG)
  #define ACTUATOR_DEBUG
#endif

// TO_ADD_CONTROL
// TODO-110 when IO moved to base.cpp; SYSTEM_FS wont need CONTROL

// TODO_ADD_SYSTEM
#if defined(SYSTEM_LOGGER_DEBUG)
  #define SYSTEM_TIME_DEBUG
  #define SYSTEM_FS_DEBUG
#endif
#if defined(SYSTEM_SD_DEBUG) || defined(SYSTEM_LITTLEFS_DEBUG)
  #define SYSTEM_FS_DEBUG
#endif
#if defined(CONTROL_LOGGERFS_DEBUG) || defined(CONTROL_GSHEETS_DEBUG)
  #define CONTROL_LOGGER_DEBUG 
#endif
#if defined(CONTROL_BLINKEN_DEBUG) || defined(CONTROL_HYSTERISIS_DEBUG) || defined(CONTROL_LOGGER_DEBUG) 
  #define CONTROL_DEBUG
#endif
// TO_ADD_SYSTEM - there is no class hierarchy
#if defined(SYSTEM_WIFI_DEBUG) || defined(SYSTEM_MQTT_DEBUG) || defined(SYSTEM_DISCOVERY_DEBUG) || defined(SYSTEM_OTA_DEBUG) || defined(SYSTEM_LORA_DEBUG) || defined(SYSTEM_OLED_DEBUG) || defined(SYSTEM_FS_DEBUG) || defined(SYSTEM_TIME_DEBUG) || defined(SYSTEM_SPI_DEBUG) || defined(SYSTEM_LORAMESHER_DEBUG)
  #define SYSTEM_DEBUG
#endif

#if defined(SENSOR_DEBUG) || defined(ACTUATOR_DEBUG) || defined(CONTROL_DEBUG) || defined(SYSTEM_DEBUG)
  #define ANY_DEBUG
#endif 
// TO_ADD_BOARD
// Note for lolin_c3_pico we are using lolin_c3_mini which seems correct except for RGB_BUILTIN not being defined
// 
// Board names in e.g. ~/.platformio/platforms/espressif32/boards/lolin_c3_mini.json
// ARDUINO_LOLIN_C3_MINI ARDUINO_LOLIN_S2_MINI ARDUINO_ESP8266_WEMOS_D1MINI ARDUINO_ESP8266_WEMOS_D1MINIPRO ARDUINO_ESP8266_ESP01
// These boards use a more generic board definition so need defining in platformio.dev
// LilyGo HiGrow uses esp32dev which defines ARDUINO_ESP32_DEV TODO-140 define something
// Sonoff uses sonoff_basic which defines ARDUINO_ESP8266_SONOFF_BASIC
// ttgo-lora32-v21 defines ARDUINO_TTGO_LoRa32_v21new; but also need whether SX1276 or SX1278 so TODO-140 define that
// And in the arduino core code ESP32 or ESP8266
//
// And in sdkconfig.h which is included by <Arduino.h> 
// CONFIG_IDF_TARGET_ESP32C3 etc
// CONFIG_IDF_TARGET or CONFIG_ARDUINO_VARIANT = esp32c3 etc
//
// Amd in pins_arduino.h 
// LED_BUILTIN; uart TX RX; i2c SDA SCL; spi SS MOSI MISO SCK; 
// 

// shields compatible with D1 and its ESP8266 not C-pico which has same pin layout but different availability esp of analog
// Note there are at least three versions 
// V2 (Green - long - has SPI socket, battery;
// V3 Blue with external antennea
// V4 With I2C but no external antenna

#if defined(ARDUINO_ESP8266_WEMOS_D1MINILITE) || defined(ARDUINO_ESP8266_WEMOS_D1MINIPRO) || defined(ARDUINO_ESP8266_WEMOS_D1MINI) || defined(ARDUINO_ESP8266_WEMOS_D1WROOM02) || defined(ARDUINO_ESP8266_WEMOS_D1R1)
  #define ESP8266_D1
#endif

// Difference between SX1276 and SX1278 based boards is by defined SYSTEM_LORAMESHER_MODULE in platformio.ini
#if defined(ARDUINO_TTGO_LoRa32_v21new) || defined(ARDUINO_TTGO_LoRa32_v2) || defined(ARDUINO_TTGO_LoRa32_V1)
  #define ARDUINO_TTGO_LoRa32
#endif

// Define boards which have LoRa and should include LoRaMesher automatically
#if defined(SYSTEM_LORAMESHER_BAND) // || defined(ARDUINO_TTGO_LoRa32) || defined(ARDUINO_LILYGO_T3_S3_V1_X) || defined(ARDUINO_heltec_wifi_lora_32_V3) 
  #define SYSTEM_LORAMESHER_WANT
#endif


// To specify a language (for the WiFi portal) #define all the ones you want, otherwise it supports the _ALL lsit which is currerntly EN, NL, DE, ID 
#if \
   !defined LANGUAGE_EN \
&& !defined LANGUAGE_NL \
&& !defined LANGUAGE_DE \
&& !defined LANGUAGE_ID
    #define LANGUAGE_ALL
#endif

// Always defined currently,
#define SYSTEM_WIFI_WANT  // currently always wanted

// Define boards which have built in OLED and should include automatically
#if defined(ARDUINO_TTGO_LoRa32) || defined(ARDUINO_LILYGO_T3_S3_V1_X) || defined(ARDUINO_heltec_wifi_lora_32_V3) || defined(OLED_SDA)
  #define SYSTEM_OLED_WANT
#endif

#if defined(ARDUINO_heltec_wifi_lora_32_V3)
  #define BUTTON_BUILTIN (0)
#endif

// A number of sensors will want to default to the boards I2C, 
// But some boards have the pre-defined SDA and SCL wrong
// So .... these settings define I2C_SDA and I2C_SCL so that a library can be explicit about 
// using something other than the default
#if !((defined I2C_WIRE) && defined(I2C_SDA))
  #ifdef LILYGOHIGROW
    // TODO-115 note there could be conflicts with other use of I2C and the Wire.h header which I think is where "Wire" is defined
    // I think this is a lilygo specific thing - need to check with BH1750 on other boards
    #define I2C_SDA (25)
    #define I2C_SCL (26)
    #define I2C_WIRE Wire
  #elif defined(ARDUINO_LILYGO_T3_S3_V1_X)
    // The T2_S3 has a small pair of connectors, same form factor as Lolin but different pins, using SDA1=10 SCL1=21 or SDA=TX=43 SCL=RX=44
    #define I2C_WIRE Wire // Use Wire1 for the OLED
    #define I2C_SDA TX // 43
    #define I2C_SCL RX  // 44
    // Cant get it to work on this socket, read somewhere about board surgery (resistor removal or addition) required but cant find the reference now
    //#define I2C_SDA SDA1 // 10
    //#define I2C_SCL SCL1  // 21
  #else // confirmed for ARDUINO_TTGO_LoRa32_v2, ARDUINO_TTGO_LoRa32_v21new
    // Use system defined ones
    #define I2C_WIRE Wire
    #define I2C_SDA SDA
    #define I2C_SCL SCL
  #endif
#endif


// Pin definitions that should really be in the variant files could submit PR
#ifdef ARDUINO_LOLIN_S2_MINI
  #define BUILTIN_BUTTON 0 // This is GPIO0, on pin 5, 
#endif


#endif // _SETTINGS_H
