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
#endif // ACTUATOR_BLINKIN_PIN

#ifndef ACTUATOR_LEDBUILTIN_BRIGHTNESS
  #define ACTUATOR_LEDBUILTIN_BRIGHTNESS 64
#endif

namespace aLedbuiltin {

bool value;  // 1 for on, 0 for off.

void set(int v) {
  value = v;
  #ifdef ACTUATOR_LEDBUILTIN_DEBUG
    Serial.print(F("\nSetting LED to ")); Serial.println(value);
  #endif // ACTUATOR_LEDBUILTIN_DEBUG
  #ifdef LOLIN_C3_PICO // Lolon C3 doesnt have RGB_BUILTIN defined so digitalWrite doesnt work correctly
    uint8_t brightness = v ? ACTUATOR_LEDBUILTIN_BRIGHTNESS : 0;
    neopixelWrite(ACTUATOR_LEDBUILTIN_PIN,brightness,brightness,brightness);   // Note this is g,r,b NOT r g b on Lolin
  #else // LOLIN_C3_PICO
    digitalWrite(ACTUATOR_LEDBUILTIN_PIN, v ? LOW : HIGH); // LED pin is inverted, at least on Lolin D1 Mini
  #endif // LOLIN_C3_PICO
}

//TODO29 maybe should be &topic
String *topic;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void messageReceived(String &topic, String &payload) {
#pragma GCC diagnostic pop
  uint8_t v = payload.toInt(); // Copied to pin in the loop 
  #ifdef ACTUATOR_LEDBUILTIN_DEBUG
    Serial.print(F("aLedbuiltin received "));
    Serial.println(v);
  #endif
  set(v);
}

void setup() {           
  topic = new String(*xDiscovery::topicPrefix + ACTUATOR_LEDBUILTIN_TOPIC);
  // initialize the digital pin as an output.
  pinMode(ACTUATOR_LEDBUILTIN_PIN, OUTPUT);
  #ifdef ACTUATOR_LEDBUILTIN_DEBUG
    // There is a lot of possible debugging because this is surprisingly hard i.e. non-standard across boards! 
    Serial.print(__FILE__); Serial.print(F(" LED on ")); Serial.println(ACTUATOR_LEDBUILTIN_PIN);
    //Serial.print(F(" INPUT=")); Serial.print(INPUT); 
    //Serial.print(F(" OUTPUT=")); Serial.print(OUTPUT); 
    //Serial.print(F(" INPUT_PULLUP=")); Serial.print(INPUT_PULLUP); 
    Serial.print(F(" HIGH=")); Serial.print(HIGH); 
    Serial.print(F(" LOW=")); Serial.print(LOW);   
    // Supposed to be defined, but known problem that not defined on Lolin C3 pico; 
    #ifdef RGB_BUILTIN
      Serial.print(F(" RGB_BUILTIN=")); Serial.print(RGB_BUILTIN);
    #endif
    Serial.println();
  #endif // ACTUATOR_BLINKIN_DEBUG
  xMqtt::subscribe(*topic, *messageReceived);
}

// void loop() { }

} //namespace aLEDBUILTIN
#endif // ACTUATOR_LEDBUILTIN_WANT
