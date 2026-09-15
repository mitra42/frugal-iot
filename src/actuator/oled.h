// Deep Sleep issues: the panel is re-initialised in setup() and blank during the sleep.
/* Frugal IoT - OLED Display hanler
 * This is a port of code from demo for TTGO Lora board - expand as needed
 * 
 * See https://github.com/mitra42/frugal-iot/issues/149 
 * 
 * Small OLEDs are a pain to debug, there are 4 variables that are intertwined in failure cases
 * Writing outside the offset is invisible and there is sometimes a multiple change required
 *
 * On the one I'm debugging - HW675 and I suspect its similar on others.
 * There are 4 interlinked variables - ACTUATOR_OLED_OFFSET_X; ACTUATOR_OLED_WIDTH; ACTUATOR_OLED_OFFSET_Y and ACTUATOR_OLED_HEIGHT. 
 * The ACTUATOR_OLED_OFFSET_Y doesn't matter, as long as the height is correct (20 or 40 if set the multiplex & compins)
 * and the display is created with a Y that is exactly offset+height 
 * The ACTUATOR_OLED_OFFSET_X is critical and has to be 28. The height is 20 which doesnt make a lot of sense given this is supposedly a 72*40 display
 */
 

#ifndef ACTUATOR_OLED_H
#define ACTUATOR_OLED_H
#include "_settings.h"
#ifdef ACTUATOR_OLED_WANT
#include "system/base.h" // For System_Base class
#include "system/io.h"
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
  #define ACTUATOR_OLED_IS_SSD1306 
#elif defined(ARDUINO_TTGO_LoRa32_v2) || defined(ARDUINO_TTGO_LoRa32_v21new)
  // OLED_SDA and OLED_SCL correctly defined - OLED_RST is 16 
  // These are default I2C pins for ESP32
  #define OLED_WIRE Wire  // Both Wire and Wire1 defined, but OLED_SDA=SDA=21 & OLED_SCL=SCL=22 in pins_arduino.h )
  #define OLED_RST_X -1 // OLED_RST is 16 but dont use on V21 as seems to freeze the display
  #define ACTUATOR_OLED_IS_SSD1306 
#elif defined(ARDUINO_LILYGO_T3_S3_V1_X)
  // According to https://www.espboards.dev/esp32/lilygo-t3s3-v1-0/
  // Note on other boards this is defined in  ~/.platformio/packages/framework-arduinoespressif32/variants
  #define OLED_WIRE Wire1
  #define OLED_SDA SDA
  #define OLED_SCL SCL
  #define OLED_RST_X -1 // Doesnt appear to exist on this board
  #define ACTUATOR_OLED_IS_SSD1306 
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
  #define ACTUATOR_OLED_IS_SSD1306
#elif defined(ARDUINO_T_Beam)
  // T-Beam has no built-in OLED; users wire one externally on the default I2C pins (21/22).
  // OLED_SDA and OLED_SCL are expected to be set via build_flags (e.g. -D OLED_SDA=21 -D OLED_SCL=22).
  #define OLED_WIRE Wire
  #define OLED_RST_X -1 // No dedicated reset pin on an externally-wired OLED
  #define ACTUATOR_OLED_IS_SSD1306 
#elif defined(ARDUINO_C3_OLED_72x40) 
  // Cheap OLED C3 board that is a bit odd in how display gets used
  // Code borrowed from https://github.com/AlexYeryomin/ESP32C3-OLED-72x40/blob/main/ESP32C3-OLED-72x40.ino
  #define OLED_WIRE Wire // THere is no Wire1 on C3_Pico
  #define OLED_SDA 5 // On GPIO5
  #define OLED_SCL 6 // On GPIO6
  #define OLED_RST_X -1 // Doesnt appear to exist on this board
  // Online examples suggest (30,12) or (28,24) but testing shows rows start at 0, not 24
  #define ACTUATOR_OLED_OFFSET_X 28
  #define ACTUATOR_OLED_OFFSET_Y 0
  //#define ACTUATOR_OLED_IS_SSD1306 
  #define OLED_IS_HW675
#elif defined(ACTUATOR_OLED_IS_SSD1327)
  // No board has an SSD1327 built in, so there is nothing to key a board #elif on: it is always
  // an externally wired panel, and its chip, interface and pins all come from build_flags set by
  // a board env in platformio.ini. See "SSD1327" in CLAUDE.md.
#elif !defined(OLED_WIRE) || !defined(OLED_SDA) || !defined(OLED_SCL)
  #error Undefined board for OLED
#endif

// Note there is deliberately no default chip. A board either names one in its #elif above or the
// sketch names one in build_flags; anything else stops at the #error further down. An OLED added
// to a board that has none built in is not necessarily the chip those boards usually carry, and
// guessing wrong compiles cleanly and then draws nothing.

// Note ARDUINO_LILYGO_T3_S3_V1_X not tested yet, but probably same as ARDUINO_TTGO_LoRa32
#if defined(ACTUATOR_OLED_IS_SSD1327) && !defined(ACTUATOR_OLED_HEIGHT)
  #define ACTUATOR_OLED_WIDTH 128
  #define ACTUATOR_OLED_HEIGHT 128
#endif
#ifndef ACTUATOR_OLED_HEIGHT // defined on ARDUINO_heltec_wifi_lora_32_V3
  #if defined(ARDUINO_TTGO_LoRa32) || defined(ARDUINO_LILYGO_T3_S3_V1_X) || defined(ARDUINO_heltec_wifi_lora_32_V3) || defined(ARDUINO_T_Beam) || defined( ARDUINO_heltec_wifi_lora_32_V4) // V1 or v2
    #define ACTUATOR_OLED_WIDTH 128 // OLED display width, in pixels
    #define ACTUATOR_OLED_HEIGHT 64 // OLED display height, in pixels
  #elif defined(ARDUINO_C3_OLED_72x40)
    #define ACTUATOR_OLED_WIDTH 100 // ACTUATOR_OLED_OFFSET_X(28) + visible(72);
    #define ACTUATOR_OLED_HEIGHT 40 // For 40-row mode (SETCOMPINS=0x12 override needed)
  #else
    #error Have not defined Display size
  #endif
#endif

// What chip is driving the OLED
#if defined(ACTUATOR_OLED_IS_SSD1306) || defined(OLED_IS_HW675)
  #include <Adafruit_SSD1306.h> // HW675 uses same SSD1306 driver, just with different init
#elif defined(ACTUATOR_OLED_IS_SSD1327)
  #include <Adafruit_SSD1327.h>
#else
  #error have not defined OLED chip driver
#endif

/* Portable foreground/background, because the two chips do not agree on what "white" is.
 *
 * The SSD1306 is one bit per pixel and SSD1306_WHITE is 1; the SSD1327 is four bits of greyscale
 * and SSD1327_WHITE is 0xF. Controls must use these rather than either chip's own constant - a
 * control written with SSD1306_WHITE compiles perfectly against an SSD1327 and draws in the
 * darkest grey available, i.e. invisibly.
 */
#if defined(ACTUATOR_OLED_IS_SSD1306) || defined(OLED_IS_HW675)
  #define OLED_FG SSD1306_WHITE
  #define OLED_BG SSD1306_BLACK
#elif defined(ACTUATOR_OLED_IS_SSD1327)
  #define OLED_FG SSD1327_WHITE
  #define OLED_BG SSD1327_BLACK
#else
  #error have not defined OLED colours for this chip
#endif

// Default offsets to 0 for boards without a pixel offset; adding 0 compiles away.
#ifndef ACTUATOR_OLED_OFFSET_X
  #define ACTUATOR_OLED_OFFSET_X 0
#endif
#ifndef ACTUATOR_OLED_OFFSET_Y
  #define ACTUATOR_OLED_OFFSET_Y 0
#endif

/* Wrapping the driver so that (0,0) always means the first visible pixel.
 *
 * There is one wrapper per chip rather than a template, because which methods need the offset
 * depends on what the driver overrides, and getting that wrong double-offsets silently:
 *
 *  - Adafruit_SSD1306 bypasses drawPixel in its own fast-path drawFastHLine, drawFastVLine and
 *    fillRect, so all four need offsetting here.
 *  - Adafruit_SSD1327 goes through Adafruit_GrayOLED, which overrides NOTHING but drawPixel -
 *    the other three fall through to Adafruit_GFX's generic versions, which themselves call
 *    drawPixel. Offsetting them here as well would apply the offset twice.
 *
 * drawLine and print/drawChar route through drawPixel on both, so they are covered either way.
 *
 * width()/height() return the buffer size, not the visible area - use visibleWidth()/Height().
 */
#if defined(ACTUATOR_OLED_IS_SSD1306) || defined(OLED_IS_HW675)
class OLED_Offset : public Adafruit_SSD1306 {
  public:
    OLED_Offset(uint16_t w, uint16_t h, TwoWire* twi, int8_t rst_pin)
      : Adafruit_SSD1306(w, h, twi, rst_pin) {}
    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        Adafruit_SSD1306::drawPixel(x + ACTUATOR_OLED_OFFSET_X, y + ACTUATOR_OLED_OFFSET_Y, color);
    }
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override {
        Adafruit_SSD1306::drawFastHLine(x + ACTUATOR_OLED_OFFSET_X, y + ACTUATOR_OLED_OFFSET_Y, w, color);
    }
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override {
        Adafruit_SSD1306::drawFastVLine(x + ACTUATOR_OLED_OFFSET_X, y + ACTUATOR_OLED_OFFSET_Y, h, color);
    }
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
        Adafruit_SSD1306::fillRect(x + ACTUATOR_OLED_OFFSET_X, y + ACTUATOR_OLED_OFFSET_Y, w, h, color);
    }
    int16_t visibleWidth()  const { return ACTUATOR_OLED_WIDTH  - ACTUATOR_OLED_OFFSET_X; }
    int16_t visibleHeight() const { return ACTUATOR_OLED_HEIGHT - ACTUATOR_OLED_OFFSET_Y; }
};
#elif defined(ACTUATOR_OLED_IS_SSD1327)
class OLED_Offset : public Adafruit_SSD1327 {
  public:
    using Adafruit_SSD1327::Adafruit_SSD1327; // I2C, software SPI and hardware SPI forms
    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        Adafruit_SSD1327::drawPixel(x + ACTUATOR_OLED_OFFSET_X, y + ACTUATOR_OLED_OFFSET_Y, color);
    }
    // Deliberately NOT overriding drawFastHLine/drawFastVLine/fillRect - see above
    int16_t visibleWidth()  const { return ACTUATOR_OLED_WIDTH  - ACTUATOR_OLED_OFFSET_X; }
    int16_t visibleHeight() const { return ACTUATOR_OLED_HEIGHT - ACTUATOR_OLED_OFFSET_Y; }
};
#else
  #error have not defined an OLED wrapper for this chip
#endif

class Actuator_OLED : public System_Base {
  public:
    /* The driver, as the concrete type this build selected.
     *
     * Compile-time rather than a virtual base: a board has exactly one display, so the choice is
     * known when the firmware is built, and there is no reason to pay a vtable and an indirection
     * per call on a device where the redraw is already the expensive part. Controls should hold it
     * as `auto*` and draw in OLED_FG/OLED_BG, which keeps them source-compatible across both chips.
     */
    OLED_Offset display;
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
    void drawXbox(const int16_t x0, const int16_t y0, const int16_t x1, const int16_t y1, const uint16_t color);
    TwoWire* wire;
};



#endif // ACTUATOR_OLED_WANT
#endif // ACTUATOR_OLED_H