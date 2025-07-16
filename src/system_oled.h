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

// TODO-149 check which headers needed
//#include <Arduino.h>
//#include <Wire.h> // For I2C
//#include <Adafruit_GFX.h> // For graphics functions

// Note there are differences between the TTGO LoRa V1 and V2 boards
// On Arduino the board definitions get these pins right - not checked yet on PlatformIO
#if defined(ARDUINO_TTGO_LoRa32_v1)
  // These are NOT default I2C pins for ESP32 on the V1 board but OLED_SDA and OLED_SCL correctly defined 
  #define OLED_RST_X OLED_RST // Crashes on V2
#elif defined(ARDUINO_TTGO_LoRa32_v2) || defined(ARDUINO_TTGO_LoRa32_v21new)
  // OLED_SDA and OLED_SCL correctly defined - OLED_RST is 16 
  // These are default I2C pins for ESP32
  #define OLED_RST_X -1 // OLED_RST is 16 but dont use on V21 as seems to freeze the display
#else
  #error "Unsupported OLED display configuration. Please define a new BOARD"
#endif
#if defined(ARDUINO_TTGO_LoRa32) // V1 or v2
  #include <Adafruit_SSD1306.h> // For OLED display
  #define SCREEN_WIDTH 128 // OLED display width, in pixels
  #define SCREEN_HEIGHT 64 // OLED display height, in pixels
#endif


class System_OLED : public System_Base {
  public:
    Adafruit_SSD1306 display; // OLED display object TODO-138 parameterize this and depend on board
    System_OLED(); // Constructor
    void setup(); // Setup function to initialize the display
    //void infrequently(); // Infrequent tasks, e.g., every 10 seconds
    //void periodically(); // Periodic tasks, e.g., every minute
    //void frequently(); // Frequent tasks, e.g., every 10 ms
    // TODO-149 check if needed
    void displayMessage(const String &message); // Function to display a message on the OLED
};


#endif // SYSTEM_OLED_WANT
#endif // SYSTEM_OLED_H