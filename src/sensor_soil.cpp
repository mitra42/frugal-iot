/*
  Sensor Soil
  Read from a pin and return as sSoil::Value

Configuration options
Optional with defaults: SENSOR_SOIL_PIN=4 SENSOR_SOIL_MS=15000 SENSOR_SOIL_TOPIC=soil SENSOR_SOIL_0=3200 SENSOR_SOIL_100=1300 
Optional undefined: SENSOR_SOIL_SMOOTH

On C3 - pin 0,1,4 works  5 gets error message  3 is Vbatt. 2 just reads 4095; 8,10 just reads 0; 7 reads 0 ad seems connected to LED

*/

#include "_settings.h"  // Settings for what to include etc
#ifdef SENSOR_SOIL_WANT

#include <Arduino.h>
#include "sensor_soil.h"
#include "system_mqtt.h"

Sensor_Soil::Sensor_Soil(const char* const id, const char * const name, const uint16_t map0_init, const uint16_t map100_init, const uint8_t pin_init, const uint8_t smooth_init, const char* color, const unsigned long ms_init, bool retain) 
  : Sensor_Analog(id, name, pin_init, smooth_init, 0, 100, color, ms_init, retain), map0(map0_init), map100(map100_init) { }

#define SENSOR_SOIL_INVALIDVALUE 0xFFFF

uint16_t Sensor_Soil::read() {
  const uint16_t x = analogRead(pin);
  #ifdef SENSOR_SOIL_DEBUG
    Serial.print("Soil sensor reading:"); Serial.println(x);
  #endif
  if (x == 4095) { // 12 bit -1 i.e. 0xFFF
    return SENSOR_SOIL_INVALIDVALUE;
  }
  // TODO-85 will want to be able to calibrate this somehow and remember calibration
  return map(x, map0, map100, 0, 100);
}
bool Sensor_Soil::valid(uint16_t newvalue) {
  return (newvalue != SENSOR_SOIL_INVALIDVALUE);
}


#endif // SENSOR_SOIL_WANT
