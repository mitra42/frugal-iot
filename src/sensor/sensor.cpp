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
// One read-only line per output, e.g. "Temperature: 21.5 C", using each output's own name and
// its `unit` if one was set. This is what most sensors want, and is why Sensor_HT no longer
// exists - printing temperature and humidity was most of what it did.
// Sensors whose captive-portal entry is *editable* - Sensor_Float (calibrate), Sensor_Soil and
// Sensor_LoadCell (tare) - override this with captive->addNumber()/addButton() instead.
void Sensor::captiveLines(AsyncResponseStream* response) {
  response->print(String(F("<p><label>")) + name + captiveValueLines() + "</label></p>");
}

// Split out so a sensor with something extra to show - an input, typically, which is not in
// `outputs` - can add to this rather than reimplement it. See Sensor_ENS160.
String Sensor::captiveValueLines() {
  String lines;
  for (auto &output : outputs) {
    lines += "<br>" + output->name + ": " + output->StringValue();
    if (output->unit) {
      lines += " ";
      lines += output->unit;
    }
  }
  return lines;
}

void Sensor::dispatch(System_Message &msg) {
  if (msg.module() == id) {
    for (auto &output : outputs) {
      output->dispatch(msg);
    }
    System_Base::dispatch(msg);
  }
}
