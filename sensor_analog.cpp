/*
  Sensor Analog
  Read from a pin and return as sAnalog::Value\

  See https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html for lots more on ESP ADCs

 Configuration options.
 SENSOR_ANALOG_REFERENCE for ESP8266 only  
 
*/

// TODO add the _ARRAY parameters as used in sensor_sht85.cpp so will read multiple analog inputs.

#include "_settings.h"  // Settings for what to include etc

// Add new analog sensors here TO_ADD_SENSOR
#if (defined(SENSOR_ANALOG_EXAMPLE_WANT) || defined(SENSOR_BATTERY_WANT))

#include <Arduino.h>
#include "sensor_analog.h"
#include "system_mqtt.h"
#include "system_discovery.h" // 


Sensor_Analog::Sensor_Analog(const uint8_t p) { pin = p; };

void Sensor_Analog::act() {
    xMqtt::messageSend(topic, value, false, 0);
}
void Sensor_Analog::set(const uint16_t v) {
  // May end up subclassing set if need to for example do a scaling here
  uint8_t vv;
  if (smooth) {
    vv = value - (value >> smooth) + v;
  } else {
    vv = v;
  }
  #ifdef SENSOR_ANALOG_DEBUG
    Serial.print(*name);
    if (smooth) { Serial.print(F(" Smoothed")); }
    Serial.print(" ");
    Serial.println(value);
  #endif // SENSOR_ANALOG_DEBUG
  if (value != vv) { // Only act if changed
    value = vv;
    act();
  }
}

// Note this is virtual, and subclassed in Sensor_Battery
uint16_t Sensor_Analog::read() {
  return analogRead(pin);
}

void Sensor_Analog::setup() {
  // initialize the analog pin as an input.
  pinMode(pin, INPUT); // I don't think this is needed ?
  #ifdef ESP8266_D1_MINI
    analogReference(SENSOR_ANALOG_REFERENCE); // TODO see TODO's in the sensor_analog.h
  #else
    //#error analogReference is board specific, appears to be undefined on ESP32C3 
  #endif
  
}

void Sensor_Analog::loop() {
  if (nextLoopTime <= millis()) {
    set(read()); // Will also send message via act()
  }
  nextLoopTime = millis() + ms;
}

#endif // SENSOR_ANALOG_WANT
// TODO-57 need to do discovery

