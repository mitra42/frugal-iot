/*
  Base class for sensors that return a float

  Note - we don't have any yet as SHT and DHT which return two floats, are subclasses of HT which is a subclass of Sensor 
*/

#include "_settings.h"  // Settings for what to include etc

#if defined(SENSOR_XYZ_WANT) || defined(SENSOR_MPQ_WANT)
  #define SENSOR_FLOAT_WANT
#endif
#ifdef SENSOR_FLOAT_WANT

#include <Arduino.h>
#include <forward_list>
#include "sensor.h"
#include "sensor_float.h"
#include "system_mqtt.h"

Sensor_Float::Sensor_Float(const char* name, const char* topicLeaf, uint8_t width, float min, float max, const char* color, const unsigned long ms_init, bool retain) 
: Sensor(name, ms_init, retain),
  output(new OUTfloat(name, 0, topicLeaf, width, min, max, color, false)),
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
String Sensor_Float::advertisement() {
  return output->advertisement(name); // Note using name of sensor not name of output (which is usually the same)
}
    
#endif // SENSOR_FLOAT_WANT
