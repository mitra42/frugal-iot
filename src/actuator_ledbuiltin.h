/* Frugal IoT - LED control 
 *
 * LEDs have some historical problems, and legacy bugs so some of this code may look weird, 
 * please be careful about un-weirding it if you do not have all the options to test on.
 * 
 * Using RBB_BUILTIN as the indicator of whether its RGB or not and using that for calls to neopixelwrite or rgbLedWrite
 * Using LED_BUILTIN for the digital pin and calls to digitalWrite
 * Use RGB_BRIGHTNESS as default brightness typically set to around 64 on bright LEDs
 * If ACTUATOR_LEDBUILTIN_INVERT is defined then digital output inverted (SONOFF-R2 and ESP8266-D1)
 * 
 * Note that for RGB Leds LED_BUILTIN=RGB_BUILTIN and is set to a higher value 
 *
 * ODDITIES KNOWN AND HOW CORRECTED
 * lolin_c3_pico board definition is missing - its provided in boards/ and defines LED_BUILTIN=RGB_BUILTIN=29 (7+SOC_GPIO_PIN_COUNT)
 * ITEAD_SONOFF and ESP8266_D1 are inverted - 1 is off, 0 is on // TODO-141 confirm
 * ITEAD_SONOFF - esp01_1m defines LED_BUILTIN on 1 but have note that its on 13 and inverted  TODO-141 check
 * Some legacy boards (Arduino Uno?) define BUILTIN_LED instead
 * 
 * Expect from platformio.ini
 * ACTUATOR_LEDBUILTIN_DEBUG - if want detailed debugging - needed surprisingly often given oddities.
 *
 * Outout from .h 
 *
 * KNOWN PINS For reference the LED is on the following pins for boards we have been working with .... 
 * TODO-ADD-BOARD please check and add below
 * Digital: ARDUINO_LOLIN_S2_MINI (15) TODO check); ARDUINO_TTGO_LoRa32 (25);
 * Inverted Digital: Sonoff: 13 (TODO-CHECK THEN DEFINE BELOW); esp8266-D1 (2); 
 * RGB: LILYGOHIGROW: 18 - but have not been able to get it to work 
 */

#ifndef ACTUATOR_LEDBUILTIN_H
#define ACTUATOR_LEDBUILTIN_H

/*
 * Sonoff: 13   
 * D1-mini and D1-mini-pro: 2 
*/

#include "actuator_digital.h" // for class Actuator_Digital

// On some older boards - esp Arduino Uno LED_BUILTIN is not defined but BUILTIN_LED=13, *but* is a constant so does not show up with ifdef
#if !defined(LED_BUILTIN) && defined(BUILTIN_LED)
  // May put a board test in here if find this is problematic on boards with no LED_BUILTIN defined because there is no LED
  #define LED_BUILTIN BUILTIN_LED // BUILTIN_LED probably a constant
#endif

#if defined(LILYGOHIGROW)
  #define LED_BUILTIN 18 // TODO test if this actually works - see notes that have failed - also try as Neopixel 
  //#define LED_BUILTIN 18+SOC_GPIO_PIN_COUNT // TODO test if this actually works - see notes that have failed - also try as Neopixel 
  #define RGB_BUILTIN LED_BUILTIN
#endif

#ifndef LED_BUILTIN
  // May need to comment this out if encounter voards with no LED as this is now always compiled
  #error "No ACTUATOR_LEDBUILTIN_PIN pin defined and no default known for this board"
#endif

// Oddity - some digital boards are inverted 
// TODO-141 reconfirm this  - including checking definition of HIGH and LOW 
// Also check which pin SONOFF is on and make sure set somewhere as LED_BUILTIN probably wrong
#if defined(ITEAD_SONOFF) || defined(ESP8266_D1) 
  #define ACTUATOR_LEDBUILTIN_INVERT 
#endif

#ifdef ARDUINO_LOLIN_C3_PICO
  #ifdef NOLONGERNEEDED_COS_IN_VARIANT_FILE
    #undef LED_BUILTIN
    //static const uint8_t LED_BUILTIN = 7+SOC_GPIO_PIN_COUNT;
    #define BUILTIN_LED  LED_BUILTIN // backward compatibility
    #define LED_BUILTIN (7+SOC_GPIO_PIN_COUNT)  // allow testing #ifdef LED_BUILTIN
    #define RGB_BUILTIN LED_BUILTIN
    #define RGB_BRIGHTNESS 64
  #endif
  // Note this has to be defined in Platformio as required to compile rgbledwrite correctly
  //#define RGB_BUILTIN_LED_COLOR_ORDER LED_COLOR_ORDER_RGB // default (or GRB) is wrong TODO-131 get fixed in variant file
#endif

class Actuator_Ledbuiltin : public Actuator_Digital {
  // Actuator_Digital value is on/off for LED
  public: 
    Actuator_Ledbuiltin(const uint8_t p, uint8_t brightness = 255, const char* color = "0xFFFFFF");
  protected:
    void act() override;
    INuint16* brightness; // Brightness of LED  0-255  // TODO-INuint8
    #ifdef RGB_BUILTIN
      INcolor* color;
    #endif
};

#endif // ACTUATOR_LEDBUILTIN_H