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

Sensor_HT::Sensor_HT(const char* const id, const char * const name, boolean retain) 
  : Sensor(id, name, retain),
    temperature(new OUTfloat(id, "temperature", "Temperature", 0, 1, 0.0, 45.0, "red", false)),
    humidity(new OUTfloat(id, "humidity", "Humidity", 0, 1, 0.0, 100.0, "blue", false))
  { }

// TODO_C++_EXPERT this next line is a completely useless one there just to stop the compiler barfing. See https://stackoverflow.com/questions/3065154/undefined-reference-to-vtable
// All subclasses will override this.   Note same issue on sensor_float and sensor_uint16 and sensor_ht
void Sensor_HT::readAndSet() { Serial.println("Sensor_HT::readAndSet must be subclassed"); }

void Sensor_HT::set(const float temp, const float humy) {
  temperature->set(temp);
  humidity->set(humy);
}

#ifdef SYSTEM_DISCOVERY_SHORT
void Sensor_HT::discover() {
  temperature->discover();
  humidity->discover();
}
#else
String Sensor_HT::advertisement() {
  return temperature->advertisement(name.c_str()) + humidity->advertisement(name.c_str()); // Note using name of sensor not name of output (which is usually the same)
}
#endif
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
