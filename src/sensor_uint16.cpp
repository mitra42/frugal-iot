/*
  Base class for sensors that have a Uint16_t value
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <forward_list>
#include "sensor.h"
#include "sensor_uint16.h"

//Sensor_Uint16::Sensor_Uint16() : Sensor() {  };
Sensor_Uint16::Sensor_Uint16(const char* const id, const char * const name, const uint8_t smooth_init, uint16_t min, uint16_t max, const char* color, bool retain)
  : Sensor(id, name, retain), smooth(smooth_init) {
    output = new OUTuint16(id, id, name, 0, min, max, color, false); //TODO-25-22apr pass color
  }
    

// All subclasses will override this.   Note same issue on sensor_float and sensor_uint16
uint16_t Sensor_Uint16::readUint16() { Serial.println(F("Sensor_Uint16::read must be subclassed")); return -1; }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
bool Sensor_Uint16::validate(uint16_t newvalue) {
  return true; // Default to true, will be subclassed e.g. for sensor_soil
}
#pragma GCC diagnostic pop

uint16_t Sensor_Uint16::convert(uint16_t v) {
  uint16_t vv;
  if (smooth) {
    vv = output->value - (output->value >> smooth) + v;
  } else {
    vv = v;
  }
  return(vv); // Set the value in the OUT object and send
}
void Sensor_Uint16::set(uint16_t vv) {
  output->set(vv);
}
void Sensor_Uint16::readValidateConvertSet() {
  // Note almost identical code in Sensor_Uint16 Sensor_Float & Sensor_Analog
  uint16_t v = readUint16();               // Read raw value from sensor
  if (validate(v)) {              // Check if its valid
    uint16_t vv = convert(v);        // Convert - e.g. scale and offset
    set(vv);                        // set - and send message
  }
  #ifdef SENSOR_FLOAT_DEBUG
    Serial.print(id); Serial.print(F(" raw:")); Serial.print(raw); Serial.print(F(" converted")); Serial.print(vv);
  #endif
}

void Sensor_Uint16::discover() {
  output->discover();
}
void Sensor_Uint16::dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet) {
  if (topicSensorId == id) {
    if (output->dispatchLeaf(topicTwig, payload, isSet)) { // True if changed
      // Nothing to do on Sensor
    }
    System_Base::dispatchTwig(topicSensorId, topicTwig, payload, isSet);
  }
}
