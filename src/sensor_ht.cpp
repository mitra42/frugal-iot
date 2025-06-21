/*
 * Base class for temperature and humidity sensors (Sensor_SHT & Sensor_DHT)
 */

#include "_settings.h"  // Settings for what to include etc

#ifdef SENSOR_HT_WANT

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

String Sensor_HT::advertisement() {
  return temperature->advertisement(name) + humidity->advertisement(name); // Note using name of sensor not name of output (which is usually the same)
}
void Sensor_HT::dispatchTwig(const String &topicSensorId, const String &leaf, const String &payload, bool isSet) {
  if (topicSensorId == id) {
    if (
      temperature->dispatchLeaf(leaf, payload, isSet) ||
      humidity->dispatchLeaf(leaf, payload, isSet)
    ) {
      // Nothing to do on Sensor
    }
  }
}

#endif // SENSOR_HT_WANT