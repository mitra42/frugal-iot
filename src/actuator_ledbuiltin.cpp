/*
  Turn the built in LED on or off

  Required from .h: ACTUATOR_LEDBUILTIN_PIN
  Optional:   ACTUATOR_LEDBUILTIN_DEBUG ACTUATOR_LEDBUILTIN_BRIGHTNESS
  Optional: BUILTIN_LED LED_BUILTIN RGB_BUILTIN - set on various boards  

  For reference the LED is on the following pins for boards we have been working with .... 
  Sonoff: 13
  
*/

#include "_settings.h"  // Settings for what to include etc

#ifdef ACTUATOR_LEDBUILTIN_WANT

#include <Arduino.h>
#include "actuator_ledbuiltin.h"
#include "actuator_digital.h"
#include "system_mqtt.h"
#include "system_discovery.h"

#ifdef LOLIN_C3_PICO
  #ifndef ACTUATOR_LEDBUILTIN_BRIGHTNESS
    #define ACTUATOR_LEDBUILTIN_BRIGHTNESS 64
  #endif
#endif // LOLIN_C3_PICO

Actuator_Ledbuiltin::Actuator_Ledbuiltin(const uint8_t p, const char* topic) : Actuator_Digital(p, topic) { }

void Actuator_Ledbuiltin::act() {
  #ifdef RGB_BUILTIN // Lolon C3 doesnt have RGB_BUILTIN defined so digitalWrite doesnt work correctly
    #error Unclear to me if boards with RGB_BUILTIN shoul used the neopixelwrie or digitalWrite (with latter doing a neopixelwrite)
  #endif
  // TO_ADD_BOARD if the board's built in LED is RGB then add to the definition in the next line
  #if defined(LOLIN_C3_PICO) 
    const uint8_t brightness = value ? ACTUATOR_LEDBUILTIN_BRIGHTNESS : 0;
    #ifdef PLATFORMIO
      neopixelWrite(ACTUATOR_LEDBUILTIN_PIN,brightness,brightness,brightness);   // Note this is r,g,b (Neopixel is g r b on Lolin)
    #else // neopixelWrite deprecated on Arduino IDE
      rgbLedWrite(ACTUATOR_LEDBUILTIN_PIN,brightness,brightness,brightness);   // Note this is r,g,b (Neopixel is g r b on Lolin)
    #endif
  #else // LOLIN_C3_PICO
    digitalWrite(ACTUATOR_LEDBUILTIN_PIN, value ? LOW : HIGH); // LED pin is inverted, at least on Lolin D1 Mini
  #endif // LOLIN_C3_PICO
}
#endif // ACTUATOR_LEDBUILTIN_WANT
