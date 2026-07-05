/*
  Base class for sensors
*/

//#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <vector>
#include "sensor/sensor.h"
#include "system/message.h"
#include "misc.h" // shouldBeDefined

Sensor::Sensor(const char* const id, const char* const name, bool r) 
: System_SensorActuator(id, name), retain(r) { }


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
