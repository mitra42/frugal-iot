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
 * lolin_c3_pico board definition is missing, most people use lolin_c3_mini board defs but that gets LED_BUILTIN and RGB_BUILTIN wrong - overridden below
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

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "actuator_ledbuiltin.h"
#include "actuator_digital.h"

#define ACTUATOR_LEDBUILTIN_WHITE "0xFFFFFF"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
Actuator_Ledbuiltin::Actuator_Ledbuiltin(const uint8_t pin, uint8_t brightness, const char* colorInit) :
  Actuator_Digital("ledbuiltin", "Built in LED", pin,  "yellow"),
  #ifdef RGB_BUILTIN
    color(new INcolor("ledbuiltin", "color", "LED color", colorInit, false)), //TODO-131 color of UX should reflect color of LED
  #endif
  brightness(new INuint16("ledbuiltin", "brightness", "Brightness", brightness, 0, 255, colorInit, false))
  { 
    #ifdef ACTUATOR_LEDBUILTIN_DEBUG
      Serial.print(F("Ledbuiltin pin=")); Serial.println(pin); 
    #endif
  }
#pragma GCC diagnostic pop

void Actuator_Ledbuiltin::setup() {
  Actuator_Digital::setup(); // Read config AFTER setup inputs
  brightness->setup();
  #ifdef ACTUATOR_LEDBUILTIN_DEBUG
    color->setup();
  #endif
}

void Actuator_Ledbuiltin::dispatchTwig(const String &topicActuatorId, const String &leaf, const String &payload, bool isSet) {
  if (topicActuatorId == id) {
    if (
      #ifdef RGB_BUILTIN
        color->dispatchLeaf(leaf, payload, isSet) ||
      #endif
      brightness->dispatchLeaf(leaf, payload, isSet)
    )
    { // True if changed
      act();
    }
    Actuator_Digital::dispatchTwig(topicActuatorId, leaf, payload, isSet); // Call parent to handle "input"
  }
}
  
void Actuator_Ledbuiltin::act() {
  #ifdef RGB_BUILTIN
    #ifdef ESP8266
      // Fix this if encounter a ESP8266 board with RGB LED
      Serial.println(F("Do not have code for RGB LED on ESP8266")); 
    #else
      // TODO-131 should set brightness based on input message 
      const uint16_t m = input->value ? brightness->value : 0;
      uint8_t r, g, b;
      if (m == 0xFF) {
        r = color->r;
        g = color->g;
        b = color->b;
      } else {
        r = (m * color->r) >> 8;
        g = (m * color->g) >> 8;
        b = (m * color->b) >> 8;
      }
      #ifdef ACTUATOR_LEDBUILTIN_DEBUG
        Serial.print(F("Actuator_Ledbuiltin::act rgb=0x")); 
        Serial.print(r, HEX); Serial.print(g,HEX); Serial.println(b, HEX); //TODO-131 0 should be "00"
      #endif
      //#ifdef PLATFORMIO
      //  // TODO check if this is really grb or should use rgb
      //  neopixelWrite(pin,g,r,b);   // Neopixel is g r b on Lolin- esp32-hal-rgb-led.c (or maybe was, but isnt any longer)
      //#else // neopixelWrite deprecated on Arduino IDE
        rgbLedWrite(pin,r,g,b);   // Note this is r,g,b (Neopixel is g r b on Lolin)
      //#endif
    #endif
  #else // !RGB_BUILTIN
    #ifdef ACTUATOR_LEDBUILTIN_INVERT
      digitalWrite(pin, input->value ? LOW : HIGH); // LED pin is inverted, at least on Lolin D1 Mini
    #else
      digitalWrite(pin, input->value ? HIGH : LOW); // LED pin is not inverted, e.g. on Lolin S3 mini
    #endif
  #endif 
}
void Actuator_Ledbuiltin::discover() {
  #ifdef RGB_BUILTIN
    color->discover();
  #endif
  brightness->discover();
  Actuator_Digital::discover();
}
