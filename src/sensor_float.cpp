/*
  Base class for sensors that return a float

  Note - we don't have any yet as SHT and DHT which return two floats, are subclasses of HT which is a subclass of Sensor 
*/

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include <forward_list>
#include "sensor.h"
#include "sensor_float.h"

Sensor_Float::Sensor_Float(const char* const id, const char * const name, uint8_t width, float min, float max, const char* color, bool retain) 
: Sensor(id, name, retain),
  output(new OUTfloat(id, id, name, 0, width, min, max, color, false)), // Note id same as sensor id
  width(width) { };

// TODO_C++_EXPERT this next line is a completely useless one there just to stop the compiler barfing. See https://stackoverflow.com/questions/3065154/undefined-reference-to-vtable
// All subclasses will override this.   Note same issue on sensor_float and sensor_uint16 and sensor_ht
float Sensor_Float::read() { Serial.println("Sensor_Float::read must be subclassed"); return -1; }

void Sensor_Float::set(const float newvalue) {
  output->set(newvalue);
}
// Can either sublass read(), and set() or subclass readAndSet() - use latter if more than one result e.g. in sensor_HT
void Sensor_Float::readAndSet() {
  set(read()); // Will also send message via output->set() in new style.
}

#ifdef SYSTEM_DISCOVERY_SHORT
void Sensor_Float::discover() {
  output->discover();
}
#else
String Sensor_Float::advertisement() {
  return output->advertisement(name.c_str()); // Note using name of sensor not name of output (which is usually the same)
}
#endif
void Sensor_Float::dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet) {
  if (topicSensorId == id) {
    if (output->dispatchLeaf(topicTwig, payload, isSet)) { // True if changed
      // Nothing to do on Sensor
    }
    System_Base::dispatchTwig(topicSensorId, topicTwig, payload, isSet);
  }
}

