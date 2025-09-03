/*
  Base class for sensors that return a float

  Note - we don't have any yet as SHT and DHT which return two floats, are subclasses of HT which is a subclass of Sensor 
*/

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include <forward_list>
#include "sensor.h"
#include "sensor_float.h"
#include "frugal_iot.h"

Sensor_Float::Sensor_Float(const char* const id, const char * const name, uint8_t width, float min, float max, const char* color, bool retain) 
: Sensor(id, name, retain),
  output(new OUTfloat(id, id, name, 0, width, min, max, color, false)), // Note id same as sensor id
  width(width) { };

// TODO_C++_EXPERT this next line is a completely useless one there just to stop the compiler barfing. See https://stackoverflow.com/questions/3065154/undefined-reference-to-vtable
// All subclasses will override this.   Note same issue on sensor_float and sensor_uint16 and sensor_ht
float Sensor_Float::readFloat() { 
  Serial.println(F("Sensor_Float::read must be subclassed")); return -1; 
}
// Check if the raw value from the sensor is valid - defaults to true, but overridden for each sensor
bool validate(float v) {
  return true;
}
// Convert sensor to actual value - this is typically overriden, for example to apply a scale. 
float convert(float v) {
  return v;
}
void Sensor_Float::set(const float newvalue) {
  output->set(newvalue);
}
void Sensor_Float::readValidateConvertSet() {
  // Note almost identical code in Sensor_Uint16 Sensor_Float & Sensor_Analog
  float v = readFloat();               // Read raw value from sensor
  if (validate(v)) {              // Check if its valid
    float vv = convert(v);        // Convert - e.g. scale and offset
    set(vv);                        // set - and send message
  }
  #ifdef SENSOR_FLOAT_DEBUG
    Serial.print(id); Serial.print(F(" raw:")); Serial.print(raw); Serial.print(F(" converted")); Serial.print(vv);
  #endif
}

void Sensor_Float::discover() {
  output->discover();
}

// Subclass this for specific fields - like max, min etc
void Sensor_Float::dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet) {
  if (topicSensorId == id) {
    if (output->dispatchLeaf(topicTwig, payload, isSet)) { // True if changed
      // Nothing to do on Sensor
    }
    System_Base::dispatchTwig(topicSensorId, topicTwig, payload, isSet);
  }
}

void Sensor_Float::captiveLines(AsyncResponseStream* response) {
  frugal_iot.captive->addNumber(response, id, "output", String(output->value,width), name, output->min, output->max);
  // Could add Tare as button - but probably want immediate response, not waiting on SEND
}
