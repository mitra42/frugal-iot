/*
  Base class for sensors
*/

//#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <vector>
#include "sensor.h"
#include "misc.h" // shouldBeDefined

Sensor::Sensor(const char* const id, const char* const name, bool r, uint8_t power3v3_pin, uint8_t power0v_pin) 
: System_Base(id, name), retain(r), power3v3(power3v3_pin), power0v(power0v_pin) { }

// Power management methods
void Sensor::powerUp() {
  // Default implementation: call System_Base method with stored pins if valid
  if (power3v3 != 0xFF || power0v != 0xFF) {
    System_Base::powerUp(power3v3, power0v);
  }
}

void Sensor::powerDown() {
  // Default implementation: call System_Base method with stored pins if valid
  if (power3v3 != 0xFF || power0v != 0xFF) {
    System_Base::powerDown(power3v3, power0v);
  }
}

void Sensor::prepare() {
  // Prepare for sleep - power down the sensor
  powerDown();
}

void Sensor::recover() {
  // Recover from sleep - power up the sensor
  powerUp();
}

// This is the main thing each sensor does periodically. 
// It can be overridden, or any of its parts can be. 
void Sensor::readValidateConvertSet() { shouldBeDefined(); }

void Sensor::periodically() {
  readValidateConvertSet();
}
void Sensor::setup() {
  powerUp(); // Ensure sensor is powered up during setup
  readConfigFromFS(); // Reads config (one of the outputs) and passes to our dispatchTwig - should be after inputs and outputs setup (probably)
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
