/*
  Base class for pretty much everything - or should be ! 
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "_base.h"
#include "sensor.h"
#include "actuator.h"
#include "control.h"

Frugal_Base::Frugal_Base() { }; // Intentionally nothing here

void Frugal_Base::setup() { }; // This will get called if no setup() in subclass 

void Frugal_Base::setupAll() {
  #ifdef SENSOR_WANT // If there are any sensors
    Sensor::setupAll();
  #endif
  #ifdef ACTUATOR_WANT // If there are any actuators
    Actuator::setupAll();
  #endif
  #ifdef CONTROL_WANT
    Control::setupAll();
  #endif
  // TODO-25 calls system.setupAll
}
void Frugal_Base::loop() { }; // This will get called if no loop() in subclass 

void Frugal_Base::loopAll() {
  Sensor::loopAll();
  Control::loopAll();
  //Actuator::loopAll(); // Currently no loops in Actuators
  //Control::loopAll(); // Currently no loops in Controls
  // TODO-25 call system;
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void Frugal_Base::dispatch(const String &topic, const String &payload) { } // Default does nothing - actuators and controls and some system will override
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void Frugal_Base::dispatchAll(const String &topic, const String &payload) { // Default does nothing - actuators and controls and some system will override
#pragma GCC diagnostic pop
}; // Class FrugalBase

