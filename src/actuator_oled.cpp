/* Frugal IoT - OLED Display hanler
 * This is a port of code from demo for TTGO Lora board - expand as needed
 * 
 * See https://github.com/mitra42/frugal-iot/issues/149
 */
#include "_settings.h"
#include <Wire.h>

#ifdef ARDUINO_C3_OLED_72x40
  TwoWire* Wire1_OLED_ptr = nullptr;  // Pointer that will be initialized at runtime
#endif

#ifdef SYSTEM_OLED_WANT
#include "actuator_oled.h"
//Libraries for OLED Display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Helper function to get or create Wire1_OLED
TwoWire* getWire1_OLED() {
  #ifdef ARDUINO_C3_OLED_72x40
    if (Wire1_OLED_ptr == nullptr) {
      Wire1_OLED_ptr = new TwoWire(1);
    }
    return Wire1_OLED_ptr;
  #else
    return &Wire;
  #endif
}

Actuator_OLED::Actuator_OLED(TwoWire* wire)
: System_Base("oled", "OLED"),
  wire(wire)
  //display(DISPLAY_WIDTH, DISPLAY_HEIGHT, wire, OLED_RST_X) // Allow code to access 
{
  wire->begin(OLED_SDA, OLED_SCL); // Note that on ARDUINO_TTGO_LoRa32_V1, this is NOT the hardware I2C pins on ESP32
  display = Adafruit_SSD1306(DISPLAY_WIDTH, DISPLAY_HEIGHT, wire, OLED_RST_X);
}

void Actuator_OLED::setup() {
  System_Base::setup();

  // TODO experimenting for heltec - parameterize this 
  #ifdef OLED_ENABLE_LOW
    pinMode(OLED_ENABLE_LOW,OUTPUT);
    digitalWrite(OLED_ENABLE_LOW,LOW);
    //delay(20);
  #endif
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
  Serial.println("XXX pre Wire begin");
  //Serial.printf("Wire pointer: %p, Wire1 address: %p\n", wire, &Wire1);
  //wire->begin(OLED_SDA, OLED_SCL); // Note that on ARDUINO_TTGO_LoRa32_V1, this is NOT the hardware I2C pins on ESP32
  Serial.println("XXX after Wire begin");
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32 (also seems to be for 128x64 and C3_OLED_72x40)
    setupFailed();
  }
  Serial.println("XXX after Display begin");
  // Some pretty boring default settings.
  display.clearDisplay();
  display.setTextColor(WHITE); // Other examples say SSD1306_WHITE
  display.setTextSize(1);
  #if defined(OLED_OFFSET_X) && defined(OLED_OFFSET_Y)
    display.setCursor(OLED_OFFSET_X, OLED_OFFSET_Y);
  #else
    display.setCursor(0,0);
  #endif
  #ifdef SYSTEM_OLED_DEBUG
    display.println("HELLO");
  #endif
  #ifdef SYSTEM_OLED_DEBUG
    Serial.println("Written to OLED");
  #endif
  display.display(); // May not strictly be needed here, but good to ensure display is ready
}

#endif // SYSTEM_OLED_WANT