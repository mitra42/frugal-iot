/* Frugal IoT - OLED Display hanler
 * This is a port of code from demo for TTGO Lora board - expand as needed
 * 
 * See https://github.com/mitra42/frugal-iot/issues/149 
 */

#ifndef SYSTEM_OLED_H
#define SYSTEM_OLED_H
#include "_settings.h"
#ifdef SYSTEM_OLED_WANT
#include "system_base.h" // For System_Base class
#include "Wire.h"

// TODO-149 check which headers needed
//#include <Arduino.h>
//#include <Wire.h> // For I2C
//#include <Adafruit_GFX.h> // For graphics functions

// Note there are differences between the TTGO LoRa V1 and V2 boards
// On Arduino the board definitions get these pins right - not checked yet on PlatformIO
#if defined(ARDUINO_TTGO_LoRa32_V1)
  // Note its not clear to me what is defined for board TTGO_LoRa32_va
  // These are NOT default I2C pins for ESP32 on the V1 board but OLED_SDA and OLED_SCL correctly defined 
  #define OLED_WIRE Wire1  // There is Wire and Wire1 defined in Wire.h and OLED_SDA = 4, OLED_SCA=15 (not same as SDA and SCL)
  #define OLED_RST_X OLED_RST // Crashes on V2
  #define OLED_IS_SSD1306 
#elif defined(ARDUINO_TTGO_LoRa32_v2) || defined(ARDUINO_TTGO_LoRa32_v21new)
  // OLED_SDA and OLED_SCL correctly defined - OLED_RST is 16 
  // These are default I2C pins for ESP32
  #define OLED_WIRE Wire  // Both Wire and Wire1 defined, but OLED_SDA=SDA=21 & OLED_SCL=SCL=22 in pins_arduino.h )
  #define OLED_RST_X -1 // OLED_RST is 16 but dont use on V21 as seems to freeze the display
  #define OLED_IS_SSD1306 
#elif defined(ARDUINO_LILYGO_T3_S3_V1_X)
  // According to https://www.espboards.dev/esp32/lilygo-t3s3-v1-0/
  // Note on other boards this is defined in  ~/.platformio/packages/framework-arduinoespressif32/variants
  #define OLED_WIRE Wire1
  #define OLED_SDA SDA
  #define OLED_SCL SCL
  #define OLED_RST_X -1 // Doesnt appear to exist on this board
  #define OLED_IS_SSD1306 
#elif defined(ARDUINO_heltec_wifi_lora_32_V3)
  // SDA=41 SCL=42 - assume that is "Wire" and used for LoRa, assume this is Wire1
  #define OLED_WIRE Wire1
  #define OLED_SDA SDA_OLED //TODO submit a PR to make this consistent across boards
  #define OLED_SCL SCL_OLED //TODO submit a PR to make this consistent across boards
  #define OLED_RST_X RST_OLED 
  #define OLED_ENABLE_LOW Vext // Need to set this low to turn on OLED
  #define OLED_IS_SSD1306 
#elif defined(ARDUINO_C3_OLED_72x40) 
  // Cheap OLED C3 board that is a bit odd in how display gets used
  // Code borrowed from https://github.com/AlexYeryomin/ESP32C3-OLED-72x40/blob/main/ESP32C3-OLED-72x40.ino
  // Note: Wire1_OLED is lazily created at runtime in actuator_oled.cpp
  extern TwoWire* getWire1_OLED();  // Function to get Wire1_OLED (created at runtime)
  #define OLED_WIRE getWire1_OLED()
  //#define OLED_WIRE Wire
  #define OLED_SDA 5 // On GPIO5
  #define OLED_SCL 6 // On GPIO6
  #define OLED_RST_X -1 // Doesnt appear to exist on this board
  // Note various examples online suggest this is 30 and Y is 12 but that doesnt work on either of these boards I got
  #define OLED_OFFSET_X 28 
  #define OLED_OFFSET_Y 24
  #define OLED_IS_SSD1306 
#elif !defined(OLED_WIRE) || !defined(OLED_SDA) || !defined(OLED_SCL)
  #error Undefined board for OLED
#endif

// Note ARDUINO_LILYGO_T3_S3_V1_X not tested yet, but probably same as ARDUINO_TTGO_LoRa32
#ifndef DISPLAY_HEIGHT // defined on ARDUINO_heltec_wifi_lora_32_V3
  #if defined(ARDUINO_TTGO_LoRa32) || defined(ARDUINO_C3_OLED_72x40) || defined(ARDUINO_LILYGO_T3_S3_V1_X) || defined(ARDUINO_heltec_wifi_lora_32_V3) // V1 or v2
    #define DISPLAY_WIDTH 128 // OLED display width, in pixels
    #define DISPLAY_HEIGHT 64 // OLED display height, in pixels
  #else
    #error Have not defined Display size
  #endif
#endif

// What chip is driving the OLED
#if defined(OLED_IS_SSD1306)
  #include <Adafruit_SSD1306.h> // For OLED display
#else
  #error have not defined OLED chip driver
#endif

class Actuator_OLED : public System_Base {
  public:
    Actuator_OLED(TwoWire* wire = &Wire); // Constructor
    #if defined(OLED_IS_SSD1306)
      Adafruit_SSD1306 display; // OLED display object TODO-138 parameterize this and depend on board
    #else
      #error need to add "display" for appropriate OLED
    #endif
    void setup() override; // Setup function to initialize the display
    //void infrequently() override; // Infrequent tasks, e.g., every 10 seconds
    //void periodically() override; // Periodic tasks, e.g., every minute
    //void loop() override; // Frequent tasks, e.g., every 10 ms
    // TODO-149 check if needed
    void displayMessage(const String &message); // Function to display a message on the OLED
  protected:
    TwoWire* wire;
};


#endif // SYSTEM_OLED_WANT
#endif // SYSTEM_OLED_H