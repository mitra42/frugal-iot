/*
  Base class for sensors
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <vector>
#include "sensor.h"
#include "system_mqtt.h"

std::vector<Sensor*> sensors; // TODO_C++_EXPERT I wanted this to be a static inside class Sensor but compiler barfs on it.


Sensor::Sensor(const char* const name, const unsigned long m, bool r) : Frugal_Base(), name(name), ms(m), retain(r) { }

void Sensor::setup() { } // Default to do nothing

void Sensor::setupAll() {
  for (Sensor* s: sensors) {
    s->setup();
  }
}

// TODO_C++_EXPERT - unclear why this is needed, all objects in "sensors" will be subclasses either Sensor_Uint16 or Sensor_Float each of which has a readAndSet method.
void Sensor::readAndSet() {
  Serial.println(F("XXX25 Shouldnt be calling Sensor::readAndSet - should be a subclass"));
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

String Sensor::advertisement() {
  return ""; // Default is to do nothing - as sensors all have different inputs/outputs and some still use old structure
}

String Sensor::advertisementAll() {
  String ad = String();
  for (Sensor* s: sensors) {
    ad += (s->advertisement());
  }
  return ad;
}


/*
At this point no dispatching for sensors as none have INCOMING messages

void Sensor::dispatchLeaf() {String &topicLeaf, String &payload }
void Sensor::dispatchLeafAll() {
  for (Sensor* s: sensors) {
    s->dispatchLeaf();
  }
}
*/
