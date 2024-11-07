/*
  Turn the built in LED on or off

  Required from .h: ACTUATOR_LEDBUILTIN_TOPIC
  Optional:  ACTUATOR_LEDBUILTIN_PIN ACTUATOR_LEDBUILTIN_DEBUG ACTUATOR_LEDBUILTIN_BRIGHTNESS
  Optional: BUILTIN_LED LED_BUILTIN RGB_BUILTIN - set on various boards  
*/

#include "_settings.h"  // Settings for what to include etc

#ifdef ACTUATOR_LEDBUILTIN_WANT

#include <Arduino.h>
#include "actuator_ledbuiltin.h"
#include "actuator_digital.h"
#include "system_mqtt.h"
#include "system_discovery.h"

// Arduinos unfortunately dont use any kind of #define for the board type so the 
// standard blinkplay fails on for example the WEMOS boards.
// Arduino_UNO Pin 13 has an LED connected on most Arduino boards but doesnt define BUILTIN_LED
// This next part is to handle some weirdnesses where early versions of ESP8266 define BUILTIN_LED instead of LED_BUILTIN
// but BUILTIN_LED responds to ifndef
// This version works on ESP8266 D1 Mini - not tested on others 
#ifndef ACTUATOR_LEDBUILTIN_PIN
  #ifdef LED_BUILTIN
    #define ACTUATOR_LEDBUILTIN_PIN LED_BUILTIN
  #else // !LED_BUILTIN
    #ifdef BUILTIN_LED
      #define ACTUATOR_LEDBUILTIN_PIN BUILTIN_LED
    #endif // BUILTIN_LED
  #endif // LED_BUILTIN
#endif // ACTUATOR_LEDBUILTIN_PIN

#ifdef LOLIN_C3_PICO
  #ifndef ACTUATOR_LEDBUILTIN_BRIGHTNESS
    #define ACTUATOR_LEDBUILTIN_BRIGHTNESS 64
  #endif
#endif // LOLIN_C3_PICO

class Actuator_Ledbuiltin : public Actuator_Digital {
  public: 
    Actuator_Ledbuiltin(const uint8_t p) : Actuator_Digital(p) { }

    virtual void act() {
      #ifdef RGB_BUILTIN // Lolon C3 doesnt have RGB_BUILTIN defined so digitalWrite doesnt work correctly
        #error Unclear to me if boards with RGB_BUILTIN shoul used the neopixelwrie or digitalWrite (with latter doing a neopixelwrite)
      #endif
      #if defined(LOLIN_C3_PICO) 
        const uint8_t brightness = value ? ACTUATOR_LEDBUILTIN_BRIGHTNESS : 0;
        neopixelWrite(ACTUATOR_LEDBUILTIN_PIN,brightness,brightness,brightness);   // Note this is g,r,b NOT r g b on Lolin
      #else // LOLIN_C3_PICO
        digitalWrite(ACTUATOR_LEDBUILTIN_PIN, value ? LOW : HIGH); // LED pin is inverted, at least on Lolin D1 Mini
      #endif // LOLIN_C3_PICO
    }
};

namespace aLedbuiltin {

Actuator_Ledbuiltin actuator_ledbuiltin(ACTUATOR_LEDBUILTIN_PIN);

// TODO-C++EXPERT I cant figure out how to pass the class Actuator_Digital.messageReceived as callback, have tried various combinstiaons of std::bind but to no success
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void messageReceived(String &topic, String &payload) {
#pragma GCC diagnostic ignored "-Wunused-parameter"
  actuator_ledbuiltin.messageReceived(topic, payload);
}

void setup() {
  #ifdef ACTUATOR_DIGITAL_DEBUG
    actuator_ledbuiltin.name = new String(F("ledbuiltin"));
  #endif // ACTUATOR_DIGITAL_DEBUG
  actuator_ledbuiltin.topic = String(*xDiscovery::topicPrefix + ACTUATOR_LEDBUILTIN_TOPIC);
  actuator_ledbuiltin.setup();
  xMqtt::subscribe(actuator_ledbuiltin.topic, *messageReceived); // TODO-C++EXPERT see comment above
}

// void loop() { }

} //namespace aLEDBUILTIN
#endif // ACTUATOR_LEDBUILTIN_WANT
