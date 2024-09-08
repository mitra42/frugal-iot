/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */

// Arduinos unfortunately dont use any kind of #define for the board type so the 
// standard blinkplay fails on for example the WEMOS boards.
// Arduino_UNO Pin 13 has an LED connected on most Arduino boards but doesnt define BUILTIN_LED
// ESP8266 has it on pin 2, but strangely BUILTIN_LED responds as ifndef !

#include <Arduino.h>
#include "actuator_blinken.h"
#include "logic_clock.h"

#ifndef BUILTIN_LED
#define BUILTIN_LED 13
#endif

namespace aBlinken {

unsigned long lastBlinkTime = 0;
const unsigned long blinkInterval = 1000; // 1 second

void setup() {                
  // Comment out this line unless debugging port numbers
#ifdef FRUGALIOT_DEBUG
  Serial.print("BUILTIN_LED="); Serial.print(BUILTIN_LED);
  Serial.print(" INPUT="); Serial.print(INPUT); 
  Serial.print(" OUTPUT="); Serial.print(OUTPUT); 
  Serial.print(" INPUT_PULLUP="); Serial.print(INPUT_PULLUP); 
  Serial.print(" HIGH="); Serial.print(HIGH); 
  Serial.print(" LOW="); Serial.print(LOW);   
  Serial.println("");
#endif
  // initialize the digital pin as an output.
  pinMode(BUILTIN_LED, OUTPUT);
}

void loop() {
  if (Clock::hasIntervalPassed(lastBlinkTime, blinkInterval)) {
        digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
        lastBlinkTime = Clock::getTime();
    }
}
} //namespace aBlinken
