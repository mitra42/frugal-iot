/*
  Sensor Analog Class & Sensor Battery
  Read from a pin and return as sAnalog::Value or as battery voltage
*/

// TODO merge with sensor_analog so have a class
// TODO add the _ARRAY parameters as used in sensor_sht85.cpp so will read multiple analog inputs.

#include "_settings.h"  // Settings for what to include etc
#ifdef SENSOR_ANALOG_WANT
#include <Arduino.h>
#include "sensor_battery.h"

// TODO figure out how to handle multiple analog input pins 

namespace sAnalog {

#ifdef SENSOR_ANALOG_MS
unsigned long nextLoopTime = 0;
#endif // SENSOR_ANALOG_MS

int value = 0;
#ifdef SENSOR_ANALOG_SMOOTH
unsigned long smoothedValue = 0;
#endif

void setup() {      
  // pinMode(SENSOR_ANALOG_PIN, INPUT); // I don't think this is needed
  #ifdef ESP8266_D1_MINI
    analogReference(SENSOR_ANALOG_REFERENCE); // TODO see TODO's in the sensor_analog.h
  #else
    #error analogReference is board specific, appears to be undefined on ESP32C3 
  #endif
  //value = 0;
}

void readSensor() {
  value = analogRead(SENSOR_ANALOG_PIN);
}

void loop() {
#ifdef SENSOR_ANALOG_MS
  if (nextLoopTime <= millis()) {
#endif // SENSOR_ANALOG_MS
    readSensor();
#ifdef SENSOR_ANALOG_SMOOTH // TODO maybe copy this to a system function
    smoothedValue = smoothedValue - (smoothedValue >> SENSOR_ANALOG_SMOOTH) + value;
#endif // SENSOR_ANALOG_SMOOTH
#ifdef SENSOR_ANALOG_DEBUG
        Serial.print(F("Analog:")); Serial.println(value);
#ifdef SENSOR_ANALOG_SMOOTH
        Serial.print(F("Smoothed Analog:")); Serial.println(smoothedValue);
#endif // SENSOR_ANALOG_SMOOTH
#endif // SENSOR_ANALOG_DEBUG
#ifdef SENSOR_ANALOG_MS
        nextLoopTime = millis() + SENSOR_ANALOG_MS;
    }
#endif // SENSOR_ANALOG_MS
}
} //namespace sAnalog
#endif // SENSOR_ANALOG_WANT
