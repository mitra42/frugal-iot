/* Frugal IoT - OLED Display hanler
 * This is a port of code from demo for TTGO Lora board - expand as needed
 * 
 * See https://github.com/mitra42/frugal-iot/issues/149 
 */

#ifndef SYSTEM_OLED_H
#define SYSTEM_OLED_H
#include "_settings.h"
#ifdef SYSTEM_OLED_WANT
// TODO-149 check which headers needed
//#include <Arduino.h>
//#include <Wire.h> // For I2C
//#include <Adafruit_GFX.h> // For graphics functions

// Note there are differences between the TTGO LoRa V1 and V2 boards
// On Arduino the board definitions get these pins right - not checked yet on PlatformIO
#if definded(TTGO_LORA_SX127X_V1)
  // These are NOT default I2C pins for ESP32
  #define OLED_SDA 4 // 21 on V2
  #define OLED_SCL 15  // 22 on V2
  #define OLED_RST 16 // Crashes on V2
#elif defined(TTGO_LORA_SX127X_V2)
  // These are default I2C pins for ESP32
  #define OLED_SDA 21 // 4 on V1
  #define OLED_SCL 22  // 15 on V1
  #define OLED_RST -1 // 16 on V1 but dont use as seems to freeze the display
#else
  #error "Unsupported OLED display configuration. Please define either TTGO_LORA_SX127X_V1 or TTGO_LORA_SX127X_V2. or define new BOARD"
#endif
#if defined(TTGO_LORA_SX127X_V1) || defined(TTGO_LORA_SX127X_V2)
  #include <Adafruit_SSD1306.h> // For OLED display
  #define SCREEN_WIDTH 128 // OLED display width, in pixels
  #define SCREEN_HEIGHT 64 // OLED display height, in pixels
#endif


// Define the OLED display dimensions
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

class System_OLED : public Frugal_Base {
  public:
    Adafruit_SSD1306 display; // OLED display object TODO-138 parameterize this and depend on board
    System_OLED(); // Constructor
    void setup(); // Setup function to initialize the display
    //void infrequent(); // Infrequent tasks, e.g., every 10 seconds
    //void periodic(); // Periodic tasks, e.g., every minute
    //void frequent(); // Frequent tasks, e.g., every 10 ms
    // TODO-149 check if needed
    void displayMessage(const String &message); // Function to display a message on the OLED
};



#endif // SYSTEM_OLED_WANT
#endif // SYSTEM_OLED_H