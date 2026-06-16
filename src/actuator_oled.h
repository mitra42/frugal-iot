/* Frugal IoT - OLED Display hanler
 * This is a port of code from demo for TTGO Lora board - expand as needed
 * 
 * See https://github.com/mitra42/frugal-iot/issues/149 
 * 
 */

#ifndef SYSTEM_OLED_H
#define SYSTEM_OLED_H
#include "_settings.h"
#ifdef SYSTEM_OLED_WANT
#include "system_base.h" // For System_Base class

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
#elif defined(ARDUINO_TTGO_LoRa32_v2) || defined(ARDUINO_TTGO_LoRa32_v21new)
  // OLED_SDA and OLED_SCL correctly defined - OLED_RST is 16 
  // These are default I2C pins for ESP32
  #define OLED_WIRE Wire  // Both Wire and Wire1 defined, but OLED_SDA=SDA=21 & OLED_SCL=SCL=22 in pins_arduino.h )
  #define OLED_RST_X -1 // OLED_RST is 16 but dont use on V21 as seems to freeze the display
#elif defined(ARDUINO_LILYGO_T3_S3_V1_X)
  // According to https://www.espboards.dev/esp32/lilygo-t3s3-v1-0/
  // Note on other boards this is defined in  ~/.platformio/packages/framework-arduinoespressif32/variants
  #define OLED_WIRE Wire1
  #define OLED_SDA SDA
  #define OLED_SCL SCL
  #define OLED_RST_X -1 // Doesnt appear to exist on this board
#elif defined(ARDUINO_heltec_wifi_lora_32_V3) || defined(ARDUINO_heltec_wifi_lora_32_V32) || defined(ARDUINO_heltec_wifi_lora_32_V4) 
  // Documentation is that V3.2 requires Vext HIGH, but i have a board which is labeled 3.2, has battery behavior like 3.2 but requires LOW on Vext
  #if defined(XXX_ARDUINO_heltec_wifi_lora_32_V32) // Note this is made up - there is no board or variant files for this significant rev yet - define in platformio.ini for now
    #define OLED_ENABLE_HIGH Vext // Need to set this high to turn on OLED
  #else // V3 or V4
    // On V4 Vext (GPIO 36) must be driven LOW to power the OLED — confirmed by testing
    #define OLED_ENABLE_LOW Vext // Need to set this low to turn on OLED
  #endif
  //On V3 or V32 SDA=41 SCL=42 - assume that is "Wire" and used for sensors
  #define OLED_WIRE Wire1
  #define OLED_SDA SDA_OLED
  #define OLED_SCL SCL_OLED
  #define OLED_RST_X RST_OLED
#elif defined(ARDUINO_T_Beam)
  // T-Beam has no built-in OLED; users wire one externally on the default I2C pins (21/22).
  // OLED_SDA and OLED_SCL are expected to be set via build_flags (e.g. -D OLED_SDA=21 -D OLED_SCL=22).
  #define OLED_WIRE Wire
  #define OLED_RST_X -1 // No dedicated reset pin on an externally-wired OLED
#elif !defined(OLED_WIRE) || !defined(OLED_SDA) || !defined(OLED_SCL)
  #error Undefined board for OLED
#endif

// Note ARDUINO_LILYGO_T3_S3_V1_X not tested yet, but probably same as ARDUINO_TTGO_LoRa32
#ifndef DISPLAY_HEIGHT // defined on ARDUINO_heltec_wifi_lora_32_V3
  #if defined(ARDUINO_TTGO_LoRa32) || defined(ARDUINO_LILYGO_T3_S3_V1_X) || defined(ARDUINO_heltec_wifi_lora_32_V3) || defined(ARDUINO_T_Beam) || defined( ARDUINO_heltec_wifi_lora_32_V4) // V1 or v2
    #define DISPLAY_WIDTH 128 // OLED display width, in pixels
    #define DISPLAY_HEIGHT 64 // OLED display height, in pixels
  #else
    #error Have not defined Display size
  #endif
#endif
// What chip is driving the OLED
#if defined(ARDUINO_TTGO_LoRa32) || defined(ARDUINO_LILYGO_T3_S3_V1_X) || defined(ARDUINO_heltec_wifi_lora_32_V3) || defined(ARDUINO_heltec_wifi_lora_32_V4)  || defined(ARDUINO_T_Beam) // V1 or v2
  #include <Adafruit_SSD1306.h> // For OLED display
#else
  #error have not defined OLED chip driver
#endif

class Actuator_OLED : public System_Base {
  public:
    #if defined(ARDUINO_TTGO_LoRa32) || defined(ARDUINO_LILYGO_T3_S3_V1_X) || defined(ARDUINO_heltec_wifi_lora_32_V3) || defined(ARDUINO_heltec_wifi_lora_32_V4) || defined(ARDUINO_T_Beam) // V1 or v2
      // Note SD1315 is register compatible with SD1306
      Adafruit_SSD1306 display; // OLED display object TODO-138 parameterize this and depend on board
    #else
      #error need to add "display" for appropriate OLED
    #endif
    Actuator_OLED(TwoWire* wire = &Wire); // Constructor
    void setup() override; // Setup function to initialize the display
    //void infrequently() override; // Infrequent tasks, e.g., every 10 seconds
    //void periodically() override; // Periodic tasks, e.g., every minute
    //void loop() override; // Frequent tasks, e.g., every 10 ms
    // TODO-149 check if needed
    //void displayMessage(const String &message); // Function to display a message on the OLED
    void debug(const bool clear, const uint row, const char* s);
    void debug(const bool clear, const uint row, const uint n);

  protected:
    TwoWire* wire;
};



#endif // SYSTEM_OLED_WANT
#endif // SYSTEM_OLED_H