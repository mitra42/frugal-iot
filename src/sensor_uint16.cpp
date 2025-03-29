/*
  Base class for sensors that have a Uint16_t value
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <forward_list>
#include "sensor.h"
#include "sensor_uint16.h"
#include "system_mqtt.h"

#ifdef SENSOR_UINT16_WANT

//Sensor_Uint16::Sensor_Uint16() : Sensor() {  };
Sensor_Uint16::Sensor_Uint16(const uint8_t smooth_init, const char* topic_init, const unsigned long ms_init, bool retain)
  : Sensor(topic_init, ms_init, retain), smooth(smooth_init) {}
    

// TODO_C++_EXPERT this next line is a completely useless one there just to stop the compiler barfing. See https://stackoverflow.com/questions/3065154/undefined-reference-to-vtable
// All subclasses will override this.   Note same issue on sensor_float and sensor_uint16
uint16_t Sensor_Uint16::read() { Serial.println("Sensor_Uint16::read must be subclassed"); return -1; }

void Sensor_Uint16::set(const uint16_t newvalue) {

  // Can be copy/adapted to Sensor_Float if needed
  uint16_t vv;
  if (smooth) {
    vv = value - (value >> smooth) + newvalue;
  } else {
    vv = newvalue;
  }
  #ifdef SENSOR_UINT16_DEBUG
    Serial.print(topicLeaf);
    Serial.print(" "); Serial.println(newvalue);
    if (smooth) { Serial.print(F(" Smoothed")); Serial.print(" "); Serial.println(vv); }
    
  #endif // SENSOR_UINT16_DEBUG

  if (changed(newvalue)) {
    value = vv;
    act();
  #ifdef SENSOR_UINT16_DEBUG
  } else {
    Serial.print(topicLeaf); Serial.print(F(" unchanged ")); Serial.println(newvalue);
  #endif // SENSOR_UINT16_DEBUG
  }
}
bool Sensor_Uint16::changed(const uint16_t newvalue) {
  return (newvalue != value);
}
void Sensor_Uint16::act() {
    if (topicLeaf) {
      Mqtt->messageSend(topicLeaf, value, retain, qos); // Note messageSend will convert value to String and expand topicLeaf
    }
}
void Sensor_Uint16::readAndSet() {
    set(read()); // Will also send message via act()
}
#endif //SENSOR_UINT16_WANT
