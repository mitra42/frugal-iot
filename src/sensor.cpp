/*
  Base class for sensors
*/

//#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <vector>
#include "sensor.h"

Sensor::Sensor(const char* const id, const char* const name, bool r) 
: System_Base(id, name), retain(r) { }

// Can either sublass read(), and set() or subclass readAndSet() - use latter if more than one result e.g. in sensor_HT
void Sensor::readAndSet() {
  Serial.println(F("Sensor::readAndSet - should be subclassed"));
}

void Sensor::periodically() {
  readAndSet();
}
void Sensor::setup() {
  readConfigFromFS(); // Reads config (hostname) and passes to our dispatchTwig - should be after inputs and outputs setup (probably)
}

