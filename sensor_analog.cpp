/*
  Sensor Analog
  Read from a pin and return as sAnalog::Value\
*/

// TODO turn this into a template

#include "_settings.h"  // Settings for what to include etc
#include "_common.h"    // Main include file for Framework
#ifdef WANT_SENSOR_ANALOG
#include <Arduino.h>
#include "sensor_analog.h"
#ifdef SENSOR_ANALOG_MS
#include "system_clock.h"
#endif // SENSOR_ANALOG_MS

// TODO figure out how to handle multiple analog input pins 

namespace sAnalog {

#ifdef SENSOR_ANALOG_MS
unsigned long lastLoopTime = 0;
#endif // SENSOR_ANALOG_MS

int value = 0;
#ifdef SENSOR_ANALOG_SMOOTH
unsigned long smoothedValue = 0;
#endif

void setup() {      
  // pinMode(SENSOR_ANALOG_PIN, INPUT); // I don't think this is needed
  analogReference(SENSOR_ANALOG_REFERENCE); // TODO see TODO's in the sensor_analog.h
  //value = 0;
}

int readSensor() {
  value = analogRead(SENSOR_ANALOG_PIN);
}

void loop() {
#ifdef SENSOR_ANALOG_MS
  if (sClock::hasIntervalPassed(lastLoopTime, SENSOR_ANALOG_MS)) {
#endif // SENSOR_ANALOG_MS
    readSensor();
#ifdef SENSOR_ANALOG_SMOOTH // TODO maybe copy this to a system function
    smoothedValue = smoothedValue - (smoothedValue >> SENSOR_ANALOG_SMOOTH) + value;
#endif // SENSOR_ANALOG_SMOOTH
#ifdef SENSOR_ANALOG_DEBUG
        Serial.print("Analog:");
        Serial.println(value);
#ifdef SENSOR_ANALOG_SMOOTH
        Serial.print("Smoothed Analog:");
        Serial.println(smoothedValue);
#endif // SENSOR_ANALOG_SMOOTH
#endif // SENSOR_ANALOG_DEBUG
#ifdef SENSOR_ANALOG_MS
        lastLoopTime = sClock::getTime();
    }
#endif // SENSOR_ANALOG_MS
}
} //namespace sAnalog
#endif // WANT_SENSOR_ANALOG
