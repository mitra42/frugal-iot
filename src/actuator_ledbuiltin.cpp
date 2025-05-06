/*
  Turn the built in LED on or off

  Required from .h: ACTUATOR_LEDBUILTIN_PIN
  Optional:   ACTUATOR_LEDBUILTIN_DEBUG ACTUATOR_LEDBUILTIN_BRIGHTNESS
  Optional: BUILTIN_LED LED_BUILTIN RGB_BUILTIN - set on various boards  

  For reference the LED is on the following pins for boards we have been working with .... 
  Sonoff: 13
  LILYGOHIGROW: 18
*/

#include "_settings.h"  // Settings for what to include etc

#ifdef ACTUATOR_LEDBUILTIN_WANT

#include <Arduino.h>
#include "actuator_ledbuiltin.h"
#include "actuator_digital.h"
#include "system_mqtt.h"
#include "system_discovery.h"


Actuator_Ledbuiltin::Actuator_Ledbuiltin(const uint8_t pin, uint8_t brightness, const char* color) :
  Actuator_Digital("ledbuiltin", "Built in LED", pin,  "yellow"),
  #ifdef ACTUATOR_LEDBUILTIN_RGB
    color(new INcolor("led", "color", "LED color", color, false)), //TODO-131 color of UX should reflect color of LED
  #endif
  brightness(brightness) 
  { 
    #ifdef ACTUATOR_LEDBUILTIN_DEBUG
      Serial.print(F("Ledbuiltin pin=")); Serial.println(pin); 
    #endif
  }

void Actuator_Ledbuiltin::dispatchTwig(const String &topicActuatorId, const String &leaf, const String &payload, bool isSet) {
  if (topicActuatorId == id) {
    if (
      #ifdef ACTUATOR_LEDBUILTIN_RGB
        color->dispatchLeaf(leaf, payload, isSet) ||
      #endif
      input->dispatchLeaf(leaf, payload, isSet)
    ) { // True if changed
      inputReceived(payload);
    }
  }
}
  
void Actuator_Ledbuiltin::act() {
  #ifdef RGB_BUILTIN // Lolon C3 doesnt have RGB_BUILTIN defined so digitalWrite doesnt work correctly
    #error Unclear to me if boards with RGB_BUILTIN should use the neopixelwrie or digitalWrite (with latter doing a neopixelwrite) - I dont have a board to play with that does this.
  #endif
  #ifdef ACTUATOR_LEDBUILTIN_RGB
    #ifdef ESP8266
      // Fix this if encounter a ESP8266 board with RGB LED
      Serial.println(F("Do not have code for RGB LED on ESP8266")); 
    #else
    // TODO-131 should set brightness based on input message 
    const uint16_t m = input->value ? brightness : 0;
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
      #ifdef PLATFORMIO
        neopixelWrite(pin,g,r,b);   // Neopixel is g r b on Lolin- esp32-hal-rgb-led.c
      #else // neopixelWrite deprecated on Arduino IDE
        rgbLedWrite(pin,r,g,b);   // Note this is r,g,b (Neopixel is g r b on Lolin)
      #endif
    #endif
  #else // !ACTUATOR_LEDBUILTIN_RGB
    Serial.print("XXX S2 mini led val = "); Serial.println(input->value);
    #ifdef ACTUATOR_LEDBUILTIN_INVERT
      digitalWrite(ACTUATOR_LEDBUILTIN_PIN, input->value ? LOW : HIGH); // LED pin is inverted, at least on Lolin D1 Mini
    #else
      digitalWrite(ACTUATOR_LEDBUILTIN_PIN, input->value ? HIGH : LOW); // LED pin is not inverted, e.g. on Lolin S3 mini
    #endif
  #endif 
}
#endif // ACTUATOR_LEDBUILTIN_WANT
