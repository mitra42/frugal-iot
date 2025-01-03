/*
  Base class for sensors
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "sensor.h"
#include "system_mqtt.h"

Sensor *Sensor::first = NULL;

Sensor::Sensor() : Frugal_Base() {  // TODO-25 might set topic here
  next = first; // Will be null the first time
  first = this; // Static first points to new top of linked list
};
Sensor_Float::Sensor_Float() : Sensor() {  // TODO-25 might set topic here
};
Sensor_Uint16::Sensor_Uint16() : Sensor() {  // TODO-25 might set topic here
};

void Sensor_Float::set(float newvalue) {
  if (changed(newvalue)) {
    value = newvalue;
    act();
  }
}
void Sensor_Uint16::set(uint16_t newvalue) {
  if (changed(newvalue)) {
    value = newvalue;
    act();
  }
}
// TODO_C++_EXPERT is there a (pretty!) way to do these next two pairs of textually similar as one definition or a macro or something. 
bool Sensor_Float::changed(float newvalue) {
  return (newvalue == value);
}
bool Sensor_Uint16::changed(uint16_t newvalue) {
  return (newvalue == value);
}
void Sensor_Float::act() {
      if (topic) {
      xMqtt::messageSend(topic, value, retain, qos); // Note messageSend will convert value to String and expand topic
    }
}
void Sensor_Uint16::act() {
      if (topic) {
      xMqtt::messageSend(topic, value, retain, qos); // Note messageSend will convert value to String and expand topic
    }
}
void Sensor::setup() { } // Default to do nothing

void Sensor::setupAll() {
  Sensor *s;
  for (s = first; s; s = (Sensor*)s->next) { // We know its a Sensor* since its under this list.
    s->setup();
  }
}
