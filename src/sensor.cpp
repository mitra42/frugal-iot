/*
  Base class for sensors
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <vector>
#include "sensor.h"

Sensor::Sensor(const char* const id, const char* const name, bool r) 
: System_Base(id, name), retain(r) { }

// Can either sublass read(), and set() or subclass readAndSet() - use latter if more than one result e.g. in sensor_HT
void Sensor::readAndSet() {
  Serial.println(F("XXX25 Shouldnt be calling Sensor::readAndSet - should be a subclass"));
}

void Sensor::periodically() {
  readAndSet();
}
