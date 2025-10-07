/*
  Base class for sensors that return a float

  Note - we don't have any yet as SHT and DHT which return two floats, are subclasses of HT which is a subclass of Sensor 
*/

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include <forward_list>
#include "sensor.h"
#include "sensor_float.h"
#include "Frugal-IoT.h"

Sensor_Float::Sensor_Float(const char* const id, const char * const name, uint8_t width, float min, float max, const char* color, bool retain) 
: Sensor(id, name, retain),
  output(new OUTfloat(id, id, name, 0, width, min, max, color, false)), // Note id same as sensor id
  width(width) { 
    outputs.push_back(output);
  };

// TODO_C++_EXPERT this next line is a completely useless one there just to stop the compiler barfing. See https://stackoverflow.com/questions/3065154/undefined-reference-to-vtable
// All subclasses will override this.   Note same issue on sensor_float and sensor_uint16 and sensor_ht
float Sensor_Float::readFloat() {  shouldBeDefined(); return -1; }

// Check if the raw value from the sensor is valid - defaults to true, but overridden for each sensor
bool Sensor_Float::validate(float v) {
  return true;
}
// Convert sensor to actual value - this is typically overriden, for example to apply a scale. 
float Sensor_Float::convert(float v) {
  return v;
}
void Sensor_Float::set(const float newvalue) {
  output->set(newvalue);
}
void Sensor_Float::readValidateConvertSet() {
  // Note almost identical code in Sensor_Uint16 Sensor_Float & Sensor_Analog
  float v = readFloat();               // Read raw value from sensor
  #ifdef SENSOR_FLOAT_DEBUG
    Serial.print(id); Serial.print(F(" raw:")); Serial.print(v);
  #endif
  if (validate(v)) {              // Check if its valid
    float vv = convert(v);        // Convert - e.g. scale and offset
    set(vv);                        // set - and send message
    #ifdef SENSOR_FLOAT_DEBUG
      Serial.print(F(" converted ")); Serial.print(vv);
    #endif
  }
  #ifdef SENSOR_FLOAT_DEBUG
    Serial.println();
  #endif
}

void Sensor_Float::captiveLines(AsyncResponseStream* response) {
  frugal_iot.captive->addNumber(response, id, "output", String(output->floatValue(),(int)width), name, output->min, output->max);
  // Could add Tare as button - but probably want immediate response, not waiting on SEND
}
