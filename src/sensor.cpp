/*
  Base class for sensors
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <vector>
#include "sensor.h"
#include "system_mqtt.h"

std::vector<Sensor*> sensors; // TODO_C++_EXPERT I wanted this to be a static inside class Sensor but compiler barfs on it.

Sensor::Sensor(const char* const id, const char* const name, const unsigned long m, bool r) 
: Frugal_Base(), id(id), name(name), ms(m), retain(r) { }

void Sensor::setup() { } // Default to do nothing

void Sensor::setupAll() {
  for (Sensor* s: sensors) {
    s->setup();
  }
}

// Can either sublass read(), and set() or subclass readAndSet() - use latter if more than one result e.g. in sensor_HT
void Sensor::readAndSet() {
  Serial.println(F("XXX25 Shouldnt be calling Sensor::readAndSet - should be a subclass"));
}

void Sensor::loop() {
  if (nextLoopTime <= millis()) {
    readAndSet(); // Will also send message via act() in old style sensors, or via output->set() in new style.
    nextLoopTime = millis() + ms;
  }
}

void Sensor::loopAll() {
  for (Sensor* s: sensors) {
    s->loop();
  }
}

String Sensor::advertisement() {
  return ""; // Default is to do nothing 
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

void Sensor::dispatchLeaf() {String &topicTwig, String &payload }
void Sensor::dispatchLeafAll() {
  for (Sensor* s: sensors) {
    s->dispatchLeaf();
  }
}
*/
