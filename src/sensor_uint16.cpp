/*
  Base class for sensors that have a Uint16_t value
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <forward_list>
#include "sensor.h"
#include "sensor_uint16.h"
#ifdef SENSOR_UINT16_WANT

//Sensor_Uint16::Sensor_Uint16() : Sensor() {  };
Sensor_Uint16::Sensor_Uint16(const char* const id, const char * const name, const uint8_t smooth_init, uint16_t min, uint16_t max, const char* color, const unsigned long ms_init, bool retain)
  : Sensor(id, name, ms_init, retain), smooth(smooth_init) {
    output = new OUTuint16(id, id, name, 0, min, max, color, false); //TODO-25-22apr pass color
  }
    

// TODO_C++_EXPERT this next line is a completely useless one there just to stop the compiler barfing. See https://stackoverflow.com/questions/3065154/undefined-reference-to-vtable
// All subclasses will override this.   Note same issue on sensor_float and sensor_uint16
uint16_t Sensor_Uint16::read() { Serial.println("Sensor_Uint16::read must be subclassed"); return -1; }
bool Sensor_Uint16::valid(uint16_t newvalue) {
  return true; // Default to true, will be subclassed e.g. for sensor_soil
}
void Sensor_Uint16::set(const uint16_t newvalue) {

  // Can be copy/adapted to Sensor_Float if needed
  if (valid(newvalue)) {
    uint16_t vv;
    if (smooth) {
      vv = output->value - (output->value >> smooth) + newvalue;
    } else {
      vv = newvalue;
    }
    #ifdef SENSOR_UINT16_DEBUG
      Serial.print(topicTwig);
      Serial.print(" "); Serial.println(newvalue);
      if (smooth) { Serial.print(F(" Smoothed")); Serial.print(" "); Serial.println(vv); }
      
    #endif // SENSOR_UINT16_DEBUG

    output->set(vv); // Set the value in the OUT object and send
  }
}
/*
bool Sensor_Uint16::changed(const uint16_t newvalue) {
  return (newvalue != output->value);
}
*/
/*
void Sensor_Uint16::act() {
    if (topicTwig) {
      frugal_iot.mqtt->messageSend(topicTwig, output->value, retain, qos); // Note messageSend will convert value to String and expand topicTwig
    }
}
*/
void Sensor_Uint16::readAndSet() {
    set(read()); // Will also send message via act()
}
String Sensor_Uint16::advertisement() {
  return output->advertisement(name); // Note using name of sensor not name of output (which is usually the same)
}
void Sensor_Uint16::dispatchTwig(const String &topicSensorId, const String &leaf, const String &payload, bool isSet) {
  if (topicSensorId == id) {
    if (output->dispatchLeaf(leaf, payload, isSet)) { // True if changed
      // Nothing to do on Sensor
    }
  }
}
#endif //SENSOR_UINT16_WANT
