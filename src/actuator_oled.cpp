/* Frugal IoT - OLED Display hanler
 * This is a port of code from demo for TTGO Lora board - expand as needed
 * 
 * See https://github.com/mitra42/frugal-iot/issues/149
 * 
 */
#include "_settings.h"
#include <Wire.h>
#ifdef ACTUATOR_OLED_WANT
#include "actuator_oled.h"
//Libraries for OLED Display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Note these are RELATIVE to the offset in the .h file
#define TEST_X 0
#define TEST_Y 0
#define TEST_WIDTH (ACTUATOR_OLED_WIDTH-ACTUATOR_OLED_OFFSET_X) // ACTUATOR_OLED_WIDTH 66 too small
#define TEST_HEIGHT (ACTUATOR_OLED_HEIGHT-ACTUATOR_OLED_OFFSET_Y) // ACTUATOR_OLED_HEIGHT


Actuator_OLED::Actuator_OLED(TwoWire* wire)
: System_Base("oled", "OLED"),
  wire(wire),
  display(ACTUATOR_OLED_WIDTH, ACTUATOR_OLED_HEIGHT, wire, OLED_RST_X)
{}

void Actuator_OLED::setup() {
  System_Base::setup();

  // TODO experimenting for heltec - parameterize this 
  #ifdef OLED_ENABLE_LOW //  e.g. Heltec Lora Wifi V3
    pinMode(OLED_ENABLE_LOW,OUTPUT);
    digitalWrite(OLED_ENABLE_LOW,LOW); // This is on heltec v3, note v3.2 wants it high
    //delay(20);
  #endif
  #ifdef OLED_ENABLE_HIGH // e.g. Heltec Lora Wifi V3.2
    pinMode(OLED_ENABLE_HIGH,OUTPUT);
    digitalWrite(OLED_ENABLE_HIGH,HIGH); // This is on heltec v3, note v3.2 wants it high
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
  //Serial.printf("Wire pointer: %p, Wire1 address: %p\n", wire, &Wire1);
  wire->begin(OLED_SDA, OLED_SCL); // Note that on ARDUINO_TTGO_LoRa32_V1, this is NOT the hardware I2C pins on ESP32
  // ALso worth trying SSD1306_EXTERNALVCC to see if get better contrast but on some OLEDs (HW675) just blanks it
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32 (also seems to be for 128x64 and C3_OLED_72x40)
    setupFailed();
  } else {
    #if defined(OLED_IS_HW675) // Untested on regular SSD1306
      display.ssd1306_command(SSD1306_SETCONTRAST);
      display.ssd1306_command(0xFF); // Maximum brightness (default is 0xCF)
      display.ssd1306_command(SSD1306_SETCOMPINS);
      display.ssd1306_command(0x12); // Alternative COM config; library sends 0x02 for height!=64, but 40-row panels may need 0x12
    #endif
  }
  // Some pretty boring default settings.
  display.setTextColor(WHITE); // Other examples say SSD1306_WHITE
  display.setTextSize(1);
  display.setCursor(0, 0); // SSD1306_Offset maps (0,0) to first visible pixel via ACTUATOR_OLED_OFFSET_X/Y
  #ifdef ACTUATOR_OLED_DEBUG
    // Use this diagnostic to check if writing display, but the HELLO is turning up in non-readable part
    // If still dark: The 72×40 physical pixels are probably on a part of the controller that needs SSD1306_SETMULTIPLEX = 39 (for 40 rows), not 63 (for 64 rows). The Adafruit library uses SETCOMPINS = 0x12 (interleaved COM config) when height=64, but a 40-row panel typically needs SETCOMPINS = 0x02 (sequential). As a quick test, change ACTUATOR_OLED_HEIGHT 64 → 32 in actuator_oled.h:81 — that switches the library to sequential COM config and SETMULTIPLEX=31, which is closer to what a 40-row panel needs.
    display.fillScreen(WHITE);
    delay(500);

    // Useful sometimes to create a flash to off between old state and new write when debugging
    display.clearDisplay();
    display.display();
    delay(500);

    // Write HELLO 
    display.println("HELLO");

    // Draw a box across the screen, this helps adjust width,height and offsets
    drawXbox(TEST_X,TEST_Y, TEST_X+TEST_WIDTH-1, TEST_Y+TEST_HEIGHT-1, WHITE);    
  #endif
  #ifdef ACTUATOR_OLED_DEBUG
    Serial.println("Written to OLED");
  #endif
  display.display();
}

// Two functions useful for debugging when cannot use Serial
void Actuator_OLED::debug(const bool clear, const uint row, const char* s) {
    if (clear) { display.clearDisplay(); 
      UBaseType_t uxHighWaterMark;
      uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
      display.setCursor(0,0);
      display.print(uxHighWaterMark);
    }

    display.setCursor(0,row);
    display.setTextSize(1);
    display.print(s);
    display.display();
}
void Actuator_OLED::debug(const bool clear, const uint row, const uint n) {
    if (clear) { display.clearDisplay(); }
    display.setCursor(0,row);
    display.setTextSize(1);
    display.print(n);
    display.display();
}
void Actuator_OLED::drawXbox(const int16_t x0, const int16_t y0, const int16_t x1, const int16_t y1, const uint16_t color) {
    display.drawLine(x0, y0, x1, y1, WHITE);
    display.drawLine(x0, y0, x1, y0, WHITE);
    display.drawLine(x0, y0, x0, y1, WHITE);
    display.drawLine(x0, y1, x1, y1, WHITE);
    display.drawLine(x1, y0, x1, y1, WHITE);
    display.drawLine(x0, y1, x1, y0, WHITE);
}

/* May incorporate some of the following to get lines */
#ifdef NOTUSEDYET
  void drawThickLine(Adafruit_SSD1306& d, int x0, int y0, int x1, int y1, uint8_t thickness, uint16_t color) {
    for (int i = -(thickness / 2); i <= thickness / 2; i++) {
        // offset perpendicular to the line direction
        bool steep = abs(y1 - y0) > abs(x1 - x0);
        if (steep) {
            d.drawLine(x0 + i, y0, x1 + i, y1, color);
        } else {
            d.drawLine(x0, y0 + i, x1, y1 + i, color);
        }
    }
  }
  or 
  / Horizontal bar of height `thickness`:
  display.fillRect(x, y - thickness/2, length, thickness, WHITE);
#endif
#endif // ACTUATOR_OLED_WANT'

