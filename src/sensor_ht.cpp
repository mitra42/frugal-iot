/*
 * Frugal IoT - Base class for temperature and humidity sensors (Sensor_SHT & Sensor_DHT)
 *
 * Mitra Ardron: Sept 2024...Jun 2025
 *
 */

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "sensor.h"
#include "sensor_ht.h"
#include "misc.h" // heap_print

Sensor_HT::Sensor_HT(const char* const id, const char * const name, boolean retain) 
  : Sensor(id, name, retain),
    temperature(new OUTfloat(id, "temperature", "Temperature", 0, 1, 0.0, 45.0, "red", false)),
    humidity(new OUTfloat(id, "humidity", "Humidity", 0, 1, 0.0, 100.0, "blue", false))
  { }

// TODO_C++_EXPERT this next line is a completely useless one there just to stop the compiler barfing. See https://stackoverflow.com/questions/3065154/undefined-reference-to-vtable
// All subclasses will override this.   Note same issue on sensor_float and sensor_uint16 and sensor_ht
void Sensor_HT::readAndSet() { Serial.println(F("Sensor_HT::readAndSet must be subclassed")); }

void Sensor_HT::set(const float temp, const float humy) {
  heap_print(F("Sensor_HT::set"));
  temperature->set(temp);
  humidity->set(humy);
  heap_print(F("Sensor_HT::set after"));
}

void Sensor_HT::discover() {
  temperature->discover();
  humidity->discover();
}

void Sensor_HT::dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet) {
  if (topicSensorId == id) {
    if (
      temperature->dispatchLeaf(topicTwig, payload, isSet) ||
      humidity->dispatchLeaf(topicTwig, payload, isSet)
    ) {
      // Nothing to do on Sensor
    }
    System_Base::dispatchTwig(topicSensorId, topicTwig, payload, isSet);
  }
}

// TODO may extract this to a function in system_captive if have similar
void Sensor_HT::captiveLines(AsyncResponseStream* response) {
  response->print(String(F("<p><label>")) + name + "<br>Temperature: " + temperature->StringValue() + " C<br>Humidity: " + humidity->StringValue() + " %</label>");
}

