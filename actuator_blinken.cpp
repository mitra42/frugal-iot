/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */

// Arduinos unfortunately dont use any kind of #define for the board type so the 
// standard blinkplay fails on for example the WEMOS boards.
// Arduino_UNO Pin 13 has an LED connected on most Arduino boards but doesnt define BUILTIN_LED
// ESP8266 has it on pin 2, but strangely BUILTIN_LED responds as ifndef !

#include "_settings.h"  // Settings for what to include etc
#include "_common.h"    // Main include file for Framework

#include <Arduino.h>
#include "actuator_blinken.h"
#include "system_clock.h"

#ifndef ACTUATOR_BLINKIN_PIN
#ifndef BUILTIN_LED
#define BUILTIN_LED 13
#endif
#define ACTUATOR_BLINKIN_PIN BUILTIN_LED
#endif

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
