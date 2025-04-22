/*
  Base class for Actuators
*/

#include "_settings.h"  // Settings for what to include etc
#ifdef ACTUATOR_DIGITAL_WANT // defined in _settings.h if subclasses defined
#include <Arduino.h>
#include <vector>
#include "actuator.h"
#include "system_mqtt.h"

std::vector<Actuator*> actuators; // TODO_C++_EXPERT I wanted this to be a static inside class Actuator but compiler barfs on it.

#ifdef ACTUATOR_DEBUG
void Actuator_debug(const char * msg) {
  Serial.print(msg); 
  for (Actuator* a: actuators) {
    Serial.print(a->topicLeaf); Serial.print(" ");
  }
  Serial.println();
  delay(1000); // Allow Serial to stabilize
}
#endif // ACTUATOR_DEBUG

Actuator::Actuator(const char* name) : Frugal_Base(), name(name) { } 

void Actuator::setup() { } // does nothing - always subclassed.

void Actuator::setupAll() {
  for (Actuator* a: actuators) {
    a->setup();
  }
}

/* Zero Actuators currently need readAndSet
// TODO_C++_EXPERT - unclear why this is needed, all objects in "actuators" will be subclasses e.g. Actuator_digital each of which has a readAndSet method.
void Actuator::readAndSet() {
  Serial.println("XXX25 Shouldnt be calling Actuator::readAndSet - should be a subclass");
}
*/
/* Zero Actuators currently need loop
void Actuator::loop() {
  if (nextLoopTime <= millis()) {
    readAndSet(); // Will also send message via act()
    nextLoopTime = millis() + ms;
  }
}

void Actuator::loopAll() {
  for (Actuator* s: actuators) {
    s->loop();
  }
}
*/

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void Actuator::inputReceived(const String &payload) {
#pragma GCC diagnostic pop
  Serial.println("XXX25 Actuator::inputReceived should be subclassed");
}

void Actuator::dispatchLeaf(const String &topic, const String &payload) {
  Serial.println("XXX25 Actuator::dispatchLeaf should be subclassed");
}

void Actuator::dispatchLeafAll(const String &topicLeaf, const String &payload) {
  for (Actuator* a: actuators) {
    a->dispatchLeaf(topicLeaf, payload);
  }
}

String Actuator::advertisement() {
  return ""; // Default is to do nothing - as actuators all have different inputs/outputs
}

String Actuator::advertisementAll() {
  String ad = String();
  for (Actuator* a: actuators) {
    ad += (a->advertisement());
  }
  return ad;
}

#endif //ACTUATOR_WANT

