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

void Sensor::discover() {
  for (auto &output : outputs) {
    output->discover();
  }
}
void Sensor::dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet) {
  if (topicSensorId == id) {
    //bool changed = false; // Not needed on sensor as dont act on changes
    for (auto &output : outputs) {
      if (output->dispatchLeaf(topicTwig, payload, isSet)) { // Will send value if wiredPath changed
        //changed = true; // Shouldnt happen - changing outputs shouldnt cause process, but here for completeness.
      }; 
    }
    // Catch fields every sensor or actuator or control has - like name 
    System_Base::dispatchTwig(topicSensorId, topicTwig, payload, isSet);
  }
}
