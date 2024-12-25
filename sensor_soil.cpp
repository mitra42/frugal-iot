/*
  Sensor Soil
  Read from a pin and return as sSoil::Value

Configuration options
Optional: SENSOR_SOIL_PIN SENSOR_SOIL_MS SENSOR_SOIL_SMOOTH SENSOR_SOIL_TOPIC

On C3 - pin 0,1,4 works  5 gets error message  3 is Vbatt. 2 just reads 4095; 8,10 just reads 0; 7 reads 0 ad seems connected to LED

*/

#include "_settings.h"  // Settings for what to include etc
#ifdef SENSOR_SOIL_WANT
#include <Arduino.h>
#include "sensor_analog.h"
#include "sensor_soil.h"
#include "system_discovery.h"

#ifndef SENSOR_SOIL_PIN
  #ifdef LOLIN_C3_PICO
    #define SENSOR_SOIL_PIN 4 // Which pin to read - this will be board specific
  #else
    #ifdef ESP8266_D1_MINI
      #define SENSOR_SOIL_PIN A0 // Which pin to read - this will be board specific
    #else 
      #error Sorry no default Analog pin for your board
    #endif
  #endif
#endif

#ifndef SENSOR_SOIL_MS
  #define SENSOR_SOIL_MS 15000 // How often to read in MS
#endif

//  https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
// TODO what are the values on ESP8266 or ESP32
// TODO map between one set of REFERENCE values and the board specfic ones from the docs 
// See https://github.com/mitra42/frugal-iot/issues/60
#ifndef SENSOR_ANALOG_REFERENCE
  #ifdef ESP8266_D1_MINI
    #define SENSOR_ANALOG_REFERENCE DEFAULT // TODO not clear if / where this is used 
  #else
    #ifndef LOLIN_C3_PICO
      #error analogReference() is board dependent, review the docs and online and define 
    #endif
  #endif
#endif //  SENSOR_ANALOG_REFERENCE

Sensor_Soil::Sensor_Soil(const uint8_t p) : Sensor_Analog(p) { 
  #ifdef SENSOR_SOIL_MS
    ms = SENSOR_SOIL_MS;
  #endif
  topic = SENSOR_SOIL_TOPIC;
  #ifdef SENSOR_SOIL_DEBUG
    name = new String(F("Soil"));
  #endif // SENSOR_SOIL_DEBUG
  #ifdef SENSOR_SOIL_SMOOTH
    smooth = SENSOR_SOIL_SMOOTH;
  #endif
  #ifdef SENSOR_SOIL_MS
    ms = SENSOR_SOIL_MS;
  #endif
}

uint16_t Sensor_Soil::read() {
  const uint16_t x = analogRead(pin);
  // TODO-85 will want to be able to calibrate this somehow and remember calibration
  return map(x, 1300, 3200, 100, 0);
}

namespace sSoil {

Sensor_Soil sensor_soil(SENSOR_SOIL_PIN);  // TODO-57 will rarely be as simple as this

void setup() {
  sensor_soil.setup();
}
// TODO create a linked list of sensors, and another of actuators that can be called in loop OR use list by time
void loop() {
  sensor_soil.loop();
}

} //namespace sSoil
#endif // SENSOR_SOIL_WANT
