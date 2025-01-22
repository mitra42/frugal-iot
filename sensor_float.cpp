/*
  Base class for sensors that return a float

  Note - we don't have any yet as SHT and DHT which return two floats, are subclasses of HT which is a subclass of Sensor 
*/

#include "_settings.h"  // Settings for what to include etc

#if defined(SENSOR_XYZ_WANT) || defined(SENSOR_MPQ_WANT)
  #define SENSOR_FLOAT_WANT
#endif
#ifdef SENSOR_FLOAT_WANT

#include <Arduino.h>
#include <forward_list>
#include "sensor.h"
#include "sensor_float.h"
#include "system_mqtt.h"

Sensor_Float::Sensor_Float(const char* topic_init, const unsigned long ms_init) : Sensor(topic_init, ms_init) { };

// TODO_C++_EXPERT this next line is a completely useless one there just to stop the compiler barfing. See https://stackoverflow.com/questions/3065154/undefined-reference-to-vtable
// All subclasses will override this.   Note same issue on sensor_float and sensor_uint16 and sensor_ht
float Sensor_Float::read() { Serial.println("Sensor_Float::read must be subclassed"); return -1; }

void Sensor_Float::set(const float newvalue) {
  if (changed(newvalue)) {
    value = newvalue;
    act();
  }
}
// TODO_C++_EXPERT is there a (pretty!) way to do these next two pairs of textually similar as one definition or a macro or something. 
bool Sensor_Float::changed(const float newvalue) {
  return (newvalue == value);
}
void Sensor_Float::act() {
    if (topic) {
      Mqtt->messageSend(topic, value, retain, qos); // Note messageSend will convert value to String and expand topic
    }
}
void Sensor_Float::readAndSet() {
    set(read()); // Will also send message via act()
}
#endif // SENSOR_FLOAT_WANT
