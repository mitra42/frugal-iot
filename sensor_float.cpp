/*
  Base class for sensors
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <forward_list>
#include "sensor.h"
#include "sensor_float.h"
#include "system_mqtt.h"

#if defined(SENSOR_ANALOG_EXAMPLE_DEBUG) || defined(SENSOR_BATTERY_DEBUG) || defined(SENSOR_SOIL_DEBUG) // TODO make this generic, but LED almost always wanted
  #define SENSOR_DEBUG
#endif

Sensor_Float::Sensor_Float(const char* topic_init, const unsigned long ms_init) : Sensor(topic_init, ms_init) { };

// TODO_C++_EXPERT this next line is a completely useless one there just to stop the compiler barfing. See https://stackoverflow.com/questions/3065154/undefined-reference-to-vtable
// All subclasses will override this.   Note same issue on sensor_float and sensor_uint16 and sensor_ht
float Sensor_Float::read() { Serial.println("Sensor_Float::read must be subclassed"); return -1; }

void Sensor_Float::set(float newvalue) {
  if (changed(newvalue)) {
    value = newvalue;
    act();
  }
}
// TODO_C++_EXPERT is there a (pretty!) way to do these next two pairs of textually similar as one definition or a macro or something. 
bool Sensor_Float::changed(float newvalue) {
  return (newvalue == value);
}
void Sensor_Float::act() {
    if (topic) {
      xMqtt::messageSend(topic, value, retain, qos); // Note messageSend will convert value to String and expand topic
    }
}
void Sensor_Float::loop() {
  if (nextLoopTime <= millis()) {
    set(read()); // Will also send message via act()
    nextLoopTime = millis() + ms;
  }
}

