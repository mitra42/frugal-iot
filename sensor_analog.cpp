/*
 * Sensor Analog
 * Read from a pin and send message
 *
 * See https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html for lots more on ESP ADCs
 *
 * Configuration options.
 * Required: SENSOR_XYZ_WANT - compiled based on any of its subclasses
 * Optional: SENSOR_ANALOG_REFERENCE for ESP8266 only  
 *
 */

#include "_settings.h"  // Settings for what to include etc

// Add new analog sensors (i.e. they read the A2D converter) here. TO_ADD_SENSOR
#if (defined(SENSOR_ANALOG_EXAMPLE_WANT) || defined(SENSOR_BATTERY_WANT) || defined(SENSOR_SOIL_WANT)) 

#include <Arduino.h>
#include "sensor_analog.h"
#include "system_mqtt.h"
#include "system_discovery.h" // 


Sensor_Analog::Sensor_Analog(const uint8_t p) : Sensor_Uint16() { pin = p; }; // TODO-25 maybe add topic here

// Sensor_Uint16_t::act is good - sends with retain=false; qos=0;
// Sensor_Uint16_t::set is good - does optional smooth, compares and calls act
// Sensor_Uint16_t::loop is good - does periodic read and set

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
#endif // SENSOR_ANALOG_WANT
// TODO-57 need to do discovery

