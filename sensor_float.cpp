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

Sensor_Float::Sensor_Float() : Sensor() {  // TODO-25 might set topic here
 // Serial.println("Sensor_Float constructor");
};

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

