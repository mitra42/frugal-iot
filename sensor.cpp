/*
  Base class for sensors
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <vector>
#include "sensor.h"
#include "system_mqtt.h"

#if defined(SENSOR_ANALOG_EXAMPLE_DEBUG) || defined(SENSOR_BATTERY_DEBUG) || defined(SENSOR_SOIL_DEBUG) // TODO make this generic, but LED almost always wanted
  #define SENSOR_DEBUG
#endif

std::vector<Sensor*> sensors; // TODO_C++_EXPERT I wanted this to be a static inside class Sensor but compiler barfs on it.

#ifdef SENSOR_DEBUG
  void Sensor_debug(const char * msg) {
    Serial.print(msg); 
    for (Sensor* s: sensors) {
      Serial.print(s->topic); Serial.print(" ");
    }
    Serial.println();
    delay(1000); // Allow Serial to stabilize
#endif // SENSOR_DEBUG

Sensor::Sensor(const char* t, const unsigned long m) : Frugal_Base(), topic(t), ms(m) { 
  sensors.push_back(this);
} //TODO I think there is a way to shortcut initialization rather than code in {} do on all constructors like this 

void Sensor::setup() { } // Default to do nothing

void Sensor::setupAll() {
  for (Sensor* s: sensors) {
    s->setup();
  }
}

// TODO_C++_EXPERT - unclear why this is needed, all objects in "sensors" will be subclasses either Sensor_Uint16 or Sensor_Float each of which has a readAndSet method.
void Sensor::readAndSet() {
  Serial.println("XXX25 Shouldnt be calling Sensor::readAndSet - should be a subclass");
}

void Sensor::loop() {
  if (nextLoopTime <= millis()) {
    readAndSet(); // Will also send message via act()
    nextLoopTime = millis() + ms;
  }
}

void Sensor::loopAll() {
  for (Sensor* s: sensors) {
    s->loop();
  }
}
/*
At this point no dispatching for sensors as none have INCOMING messages

void Sensor::dispatch() {String &topic, String &payload }
void Sensor::dispatchAll() {
  for (Sensor* s: sensors) {
    s->dispatch();
  }
}
*/
