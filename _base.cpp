/*
  Base class for digital actuators 
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "_base.h"
#include "sensor.h"
#include "actuator.h"

Frugal_Base::Frugal_Base() { }; // Intentionally nothing here

void Frugal_Base::setup() { }; // TODO-25 what should go here - maybe nothing since subclasses will loop through list and no-action is a reasonable thing.

void Frugal_Base::setupAll() {
  #ifdef SENSOR_WANT
    Sensor::setupAll();
  #endif
  #ifdef ACTUATOR_WANT
    Actuator::setupAll();
  #endif
  // TODO-25 calls system; sensor; ; control.setupAll
}
void Frugal_Base::loop() { Serial.println("XXX25 erroneously calling Frugal_Base::loop"); }; // TODO-25 what should go here - maybe nothing since subclasses will loop through list and calling thsi could just mean no loop() was needed in subclass

void Frugal_Base::loopAll() {
  Sensor::loopAll();
  //Actuator::loopAll(); // Currently no loops in Actuators
  // TODO-25 call system; control.loopAll
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void Frugal_Base::dispatch(const String &topic, const String &payload) { } // Default does nothing - actuators and controls and some system will override
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void Frugal_Base::dispatchAll(const String &topic, const String &payload) {
#pragma GCC diagnostic pop
  //Sensor::dispatchAll(topic, payload); // there is no Sensor::dispatchAll as no incoming messages to sensors.
  // TODO-25 calls system; sensor; actuator; control.dispatchAll
}; // Class FrugalBase
