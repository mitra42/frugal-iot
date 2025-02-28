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
    Serial.print(a->topic); Serial.print(" ");
  }
  Serial.println();
  delay(1000); // Allow Serial to stabilize
}
#endif // ACTUATOR_DEBUG

Actuator::Actuator(const char* t) : Frugal_Base(), topic(t) { } 

void Actuator::setup() {
  Mqtt->subscribe(topic);
} 

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

void Actuator::dispatch(const String &topicpath, const String &payload) {
  if (topicpath == topic) {
    inputReceived(payload);
  }
}

void Actuator::dispatchAll(const String &topicpath, const String &payload) {
  for (Actuator* a: actuators) {
    a->dispatch(topicpath, payload);
  }
}
#endif //ACTUATOR_WANT

