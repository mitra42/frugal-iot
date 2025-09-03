/*
  Base class for sensors
*/

//#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <vector>
#include "sensor.h"

Sensor::Sensor(const char* const id, const char* const name, bool r) 
: System_Base(id, name), retain(r) { }

// This is the main thing each sensor does periodically. 
// It can be overridden, or any of its parts can be. 
void Sensor::readValidateConvertSet() {
  Serial.println(F("Sensor::readValidateConvertSet - should be subclassed"));
}

void Sensor::periodically() {
  readValidateConvertSet();
}
void Sensor::setup() {
  readConfigFromFS(); // Reads config (hostname) and passes to our dispatchTwig - should be after inputs and outputs setup (probably)
}

