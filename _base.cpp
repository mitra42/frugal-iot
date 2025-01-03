/*
  Base class for digital actuators 
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "_base.h"
#include "sensor.h"

Frugal_Base::Frugal_Base() { }; // TODO-25 what should go here - maybe nothing since subclasses will link into next

void Frugal_Base::setup() { }; // TODO-25 what should go here - maybe nothing since subclasses will loop through list

void Frugal_Base::setupAll() {
  Sensor::setupAll();
  // TODO-25 calls system; sensor; actuator; control.setupAll - not 100% clear how to do that without creating looping header dependency 
}
void Frugal_Base::loop() { }; // TODO-25 what should go here - maybe nothing since subclasses will loop through list

void Frugal_Base::loopAll() {
  Sensor::loopAll();
  // TODO-25 calls system; sensor; actuator; control.loopAll - not 100% clear how to do that without creating looping header dependency 
}
void Frugal_Base::dispatch(String &topic, String &payload) { } // Default does nothing - actuators and controls and some system will override

void Frugal_Base::dispatchAll(String &topic, String &payload) {
  //Sensor::dispatchAll(); // there is no Sensor::dispatchAll as no incoming messages to sensors.
  // TODO-25 calls system; sensor; actuator; control.dispatchAll - not 100% clear how to do that without creating looping header dependency 
}; // Class FrugalBase
