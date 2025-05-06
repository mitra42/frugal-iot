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

void Sensor::dispatchTwig(const String &topicSensorId, const String &topicLeaf, const String &payload, bool isSet) {
  // Default on sensors is to do nothing
}

void Sensor::dispatchTwigAll(const String &topicTwig, const String &payload, bool isSet) {
  uint8_t slashPos = topicTwig.indexOf('/'); // Find the position of the slash
  if (slashPos != -1) {
    String topicSensorId = topicTwig.substring(0, slashPos);       // Extract the part before the slash
    String topicLeaf = topicTwig.substring(slashPos + 1);      // Extract the part after the slash
    for (Sensor* a: sensors) {
      a->dispatchTwig(topicSensorId, topicLeaf, payload, isSet);
    }
  } else {
    Serial.println("No slash found in topic: " + topicTwig);
  }
}
