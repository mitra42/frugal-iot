/*
  Base class for sensors
*/

//#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <vector>
#include "sensor.h"
#include "system_message.h"
#include "misc.h" // shouldBeDefined

Sensor::Sensor(const char* const id, const char* const name, bool r) 
: System_Base(id, name), retain(r) { }

Sensor* Sensor::powerPins(const uint8_t power3v3, const uint8_t power0v) {
  power3v3_ = power3v3;
  power0v_ = power0v;
  return this; // For chaining
}
// Power management methods
void Sensor::powerUp() {
  // Default implementation: call System_Base method with stored pins if valid
  if (power3v3_ != 0xFF || power0v_ != 0xFF) {
    System_Base::powerUp(power3v3_, power0v_);
  }
}

void Sensor::powerDown() {
  // Default implementation: call System_Base method with stored pins if valid
  if (power3v3_ != 0xFF || power0v_ != 0xFF) {
    System_Base::powerDown(power3v3_, power0v_);
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
  readConfigFromFS(); // Reads config (one of the outputs) and passes to our dispatch - should be after inputs and outputs setup (probably)
}

void Sensor::discover() {
  for (auto &output : outputs) {
    output->discover();
  }
}
void Sensor::dispatch(System_Message &msg) {
  if (msg.module() == id) {
    for (auto &output : outputs) {
      output->dispatch(msg);
    }
    System_Base::dispatch(msg);
  }
}
