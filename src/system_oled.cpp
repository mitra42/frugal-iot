/* Frugal IoT - OLED Display hanler
 * This is a port of code from demo for TTGO Lora board - expand as needed
 * 
 * See https://github.com/mitra42/frugal-iot/issues/149 
 */
#include "_settings.h"
#ifdef SYSTEM_OLED_WANT
#include "system_oled.h"
//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

System_OLED::System_OLED(TwoWire* wire)
: System_Base("oled", "OLED"),
  wire(wire),
  display(SCREEN_WIDTH, SCREEN_HEIGHT, wire, OLED_RST_X) // Allow code to access 
{}

void System_OLED::setup() {
  System_Base::setup();
  // Nothing to read from disk so not calling readConfigFromFS 
  // Setup code here, if needed
  #if OLED_RST_X != -1 // If OLED_RST is defined, use it (e.g. on ARDUINO_TTGO_LoRa32_V1)
    //reset OLED display via software
    pinMode(OLED_RST_X, OUTPUT);
    digitalWrite(OLED_RST_X, LOW);
    delay(20);
    digitalWrite(OLED_RST_X, HIGH);
  #endif 
    //initialize OLED
  wire->begin(OLED_SDA, OLED_SCL); // Note that on ARDUINO_TTGO_LoRa32_V1, this is NOT the hardware I2C pins on ESP32
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    setupFailed();
  }
  // Some pretty boring default settings.
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.display(); // May not strictly be needed here, but good to ensure display is ready
}

#endif // SYSTEM_OLED_WANT