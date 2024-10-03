/*
  Turn the built in LED on or off

  Optional: ACTUATOR_LEDBUILTIN_TOPIC ACTUATOR_LEDBUILTIN_PIN BUILTIN_LED LED_BUILTIN ACTUATOR_LEDBUILTIN_DEBUG
*/


#include "_settings.h"  // Settings for what to include etc

#ifdef ACTUATOR_LEDBUILTIN_WANT
#include <Arduino.h>
#include "_common.h"    // Main include file for Framework
#include "actuator_ledbuiltin.h"
#ifdef ACTUATOR_LEDBUILTIN_TOPIC
#include "system_mqtt.h"
#endif

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

namespace aLedbuiltin {

bool value;

#ifdef ACTUATOR_LEDBUILTIN_TOPIC
String *topic = new String(ACTUATOR_LEDBUILTIN_TOPIC);

void messageReceived(String &topic, String &payload) {
  value = payload.toInt(); // Copied to pin in the loop 
  #ifdef CONTROL_DEMO_MQTT_DEBUG
    Serial.print("aLedbuiltin received ");
    Serial.println(value);
  #endif
}
#endif

void setup() {                
  // initialize the digital pin as an output.
  pinMode(ACTUATOR_LEDBUILTIN_PIN, OUTPUT);
#ifdef ACTUATOR_LEDBUILTIN_DEBUG
  // There is a lot of possible debugging because this is surprisingly hard i.e. non-standard across boards! 
  Serial.println(__FILE__);
  Serial.print("BUILTIN_LED="); Serial.print(BUILTIN_LED);
  //Serial.print(" INPUT="); Serial.print(INPUT); 
  //Serial.print(" OUTPUT="); Serial.print(OUTPUT); 
  //Serial.print(" INPUT_PULLUP="); Serial.print(INPUT_PULLUP); 
  //Serial.print(" HIGH="); Serial.print(HIGH); 
  //Serial.print(" LOW="); Serial.print(LOW);   
  Serial.println("");
#endif // ACTUATOR_BLINKIN_DEBUG
#ifdef ACTUATOR_LEDBUILTIN_TOPIC
  xMqtt::subscribe(*topic, *messageReceived);
#endif // ACTUATOR_LEDBUILTIN_TOPIC
}

void loop() {
  // Note, I'm presuming there is no cost/power overhead in doing this continuously.
  digitalWrite(ACTUATOR_LEDBUILTIN_PIN, aLedbuiltin::value);
}

} //namespace aLEDBUILTIN
#endif // ACTUATOR_LEDBUILTIN_WANT
