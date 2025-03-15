/*
  Turn the built in LED on or off

  Required from .h: ACTUATOR_LEDBUILTIN_PIN
  
*/

#include "_settings.h"  // Settings for what to include etc

#ifdef ACTUATOR_LEDBUILTIN_WANT

#include <Arduino.h>
#include "actuator_ledbuiltin.h"
#include "actuator_digital.h"
#include "system_mqtt.h"
#include "system_discovery.h"


Actuator_Ledbuiltin::Actuator_Ledbuiltin(const uint8_t p, const char* topic, bool rgb, uint8_t brightness) :
  Actuator_Digital(p, topic),
  rgb(rgb), brightness(brightness) { }

void Actuator_Ledbuiltin::act() {
  #ifdef RGB_BUILTIN // Lolon C3 doesnt have RGB_BUILTIN defined so digitalWrite doesnt work correctly
    #error Unclear to me if boards with RGB_BUILTIN should use the neopixelwrie or digitalWrite (with latter doing a neopixelwrite) - I dont have a board to play with that does this.
  #endif
  // TO_ADD_BOARD if the board's built in LED is RGB then add to the definition in the next line
  if (rgb) {
    const uint8_t b = value ? brightness : 0;
    #ifdef PLATFORMIO
      neopixelWrite(pin,b,b,b);   // Note this is r,g,b (Neopixel is g r b on Lolin)- esp32-hal-rgb-led.c
    #else // neopixelWrite deprecated on Arduino IDE
      rgbLedWrite(pin,b,b,b);   // Note this is r,g,b (Neopixel is g r b on Lolin)
    #endif
  } else { // just digital LED
    digitalWrite(ACTUATOR_LEDBUILTIN_PIN, value ? LOW : HIGH); // LED pin is inverted, at least on Lolin D1 Mini
  }
}
#endif // ACTUATOR_LEDBUILTIN_WANT
