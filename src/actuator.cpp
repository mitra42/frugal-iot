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

Actuator::Actuator(const char * const id, const char * const name) : Frugal_Base(), id(id), name(name) { } 

void Actuator::setup() { } // does nothing - always subclassed.

void Actuator::setupAll() {
  for (Actuator* a: actuators) {
    Serial.print("Setting up Actuator:"); Serial.print(a->name); Serial.print(" id="); Serial.println(a->id);
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

void Actuator::dispatchTwig(const String &topicActuatorId, const String &topicLeaf, const String &payload, bool isSet) {
  Serial.println("XXX25 Actuator::dispatchTwig should be subclassed");
}

void Actuator::dispatchTwigAll(const String &topicTwig, const String &payload, bool isSet) {
  Serial.print("XXX " __FILE__); Serial.println(__LINE__); delay(100);
  uint8_t slashPos = topicTwig.indexOf('/'); // Find the position of the slash
  if (slashPos != -1) {
    String topicActuatorId = topicTwig.substring(0, slashPos);       // Extract the part before the slash
    String topicLeaf = topicTwig.substring(slashPos + 1);      // Extract the part after the slash
    for (Actuator* a: actuators) {
      Serial.print("XXX " __FILE__); Serial.print(a->id); Serial.println(__LINE__); delay(100);
      a->dispatchTwig(topicActuatorId, topicLeaf, payload, isSet);
    }
  } else {
    Serial.println("No slash found in topic: " + topicTwig);
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

