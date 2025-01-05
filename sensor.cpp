/*
  Base class for sensors
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <vector>
#include "sensor.h"
#include "system_mqtt.h"

#if defined(SENSOR_ANALOG_EXAMPLE_DEBUG) || defined(SENSOR_BATTERY_DEBUG) || defined(SENSOR_SOIL_DEBUG) // TODO make this generic, but LED almost always wanted
  #define SENSOR_DEBUG
#endif

std::vector<Sensor*> sensors; //TODO-25 TODO_C++_EXPERT I wanted this to be a static inside class Sensor but compiler barfs on it.

/*
Sensor::Sensor() : Frugal_Base() {  // TODO-25 might set topic here
  sensors.push_back(this);
};
*/
uint8_t xxx25_sensorcount = 0;

void Sensor_debug(const char * msg) {
  Serial.print(msg); 
  for (Sensor* s: sensors) {
    Serial.print(s->topic); Serial.print(" ");
  }
  Serial.println();
  delay(1000); // Allow Serial to stabilize
}

Sensor::Sensor(const char* t, const unsigned long m) : Frugal_Base(), topic(t), ms(m) { 
  sensors.push_back(this);
} //TODO I think there is a way to shortcut initialization rather than code in {} do on all constructors like this 

void Sensor::setup() { } // Default to do nothing

void Sensor::setupAll() {
  for (Sensor* s: sensors) {
    s->setup();
  }
}

// TODO_C++_EXPERT - unclear why this is needed, all objects in "sensors" will be subclasses either Sensor_Uint16 or Sensor_Float each of which has a loop method.
void Sensor::loop() {
  Serial.println("XXX-25 Shouldnt be calling Sensor::loop - should be a subclass");
}

void Sensor::loopAll() {
  for (Sensor* s: sensors) {
    s->loop();
  }
}
/*
At this point no dispatching for sensors as none have INCOMING messages

void Sensor::dispatch() { } // TODO-25 should check topic Default to do nothing

void Sensor::dispatchAll() {
  for (Sensor* s: sensors) {
    s->dispatch();
  }
}
*/
