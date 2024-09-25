/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  Based on the Blinken demo in the IDE
 */


#include "_settings.h"  // Settings for what to include etc
#include "_common.h"    // Main include file for Framework

#ifdef WANT_ACTUATOR_BLINKEN
#include <Arduino.h>
#include "actuator_blinken.h"
#include "system_clock.h"

// Arduinos unfortunately dont use any kind of #define for the board type so the 
// standard blinkplay fails on for example the WEMOS boards.
// Arduino_UNO Pin 13 has an LED connected on most Arduino boards but doesnt define BUILTIN_LED
// This next part is to handle some weirdnesses where early versions of ESP8266 define BUILTIN_LED instead of LED_BUILTIN
// but BUILTIN_LED responds to ifndef
// This version works on ESP8266 D1 Mini - not tested on others 
#ifndef ACTUATOR_BLINKIN_PIN
#ifdef LED_BUILTIN
#define ACTUATOR_BLINKIN_PIN LED_BUILTIN
#else // !LED_BUILTIN
#ifdef BUILTIN_LED
#define ACTUATOR_BLINKIN_PIN BUILTIN_LED
#endif // BUILTIN_LED
#endif // LED_BUILTIN
#endif // ACTUATOR_BLINKIN_PIN

namespace aBlinken {

#ifdef ACTUATOR_BLINKEN_MS // Its actually required else will pulse LED at loop speed
unsigned long lastLoopTime = 0;
#endif // ACTUATOR_BLINKEN_MS

// TODO Copy time pattern from sensor_analog
unsigned long lastBlinkTime = 0;
const unsigned long blinkInterval = 1000; // 1 second

void setup() {                
  // initialize the digital pin as an output.
  pinMode(ACTUATOR_BLINKIN_PIN, OUTPUT);
#ifdef ACTUATOR_BLINKIN_DEBUG
  Serial.println(__FILE__);
  Serial.print("BUILTIN_LED="); Serial.print(BUILTIN_LED);
  Serial.print(" INPUT="); Serial.print(INPUT); 
  Serial.print(" OUTPUT="); Serial.print(OUTPUT); 
  Serial.print(" INPUT_PULLUP="); Serial.print(INPUT_PULLUP); 
  Serial.print(" HIGH="); Serial.print(HIGH); 
  Serial.print(" LOW="); Serial.print(LOW);   
  Serial.println("");
#endif // ACTUATOR_BLINKIN_DEBUG

}

void loop() {

#ifdef ACTUATOR_BLINKEN_MS
  if (sClock::hasIntervalPassed(lastLoopTime, ACTUATOR_BLINKEN_MS)) {
#endif // ACTUATOR_BLINKEN_MS
        digitalWrite(ACTUATOR_BLINKIN_PIN, !digitalRead(ACTUATOR_BLINKIN_PIN));
#ifdef ACTUATOR_BLINKEN_MS
        lastLoopTime = sClock::getTime();
    }
#endif // ACTUATOR_BLINKEN_MS
}

} //namespace aBlinken
#endif // WANT_ACTUATOR_BLINKEN
