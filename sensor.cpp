/*
  Base class for sensors
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <forward_list>
#include "sensor.h"
#include "system_mqtt.h"

std::forward_list<Sensor*> sensors; //TODO-25 may want to put this inside a namespace - didn't work as a static.

Sensor::Sensor() : Frugal_Base() {  // TODO-25 might set topic here
  sensors.push_front(this);
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
  for (Sensor* s: sensors) {
    s->setup();
  }
}
/*
void Sensor::loop() { } // Default to do nothing

void Sensor::loopAll() {
  for (Sensor* s: sensors) {
    s->loop();
  }
}
*/
/*
At this point no dispatching for sensors as none have INCOMING messages

void Sensor::dispatch() { } // TODO-25 should check topic Default to do nothing

void Sensor::dispatchAll() {
  for (Sensor* s: sensors) {
    s->dispatch();
  }
}
*/
