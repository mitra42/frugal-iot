/*
 * Base class for temperature and humidity sensors (Sensor_SHT & Sensor_DHT)
 */

#include "_settings.h"  // Settings for what to include etc

#ifdef SENSOR_HT_WANT

#include <Arduino.h>
#include "sensor.h"
#include "sensor_ht.h"

Sensor_HT::Sensor_HT(const char* topic_init, const char* topic2_init, const unsigned long ms_init) 
  : Sensor(topic_init, ms_init), topic2(topic2_init), temperature(0), humidity(0) { }

// TODO_C++_EXPERT this next line is a completely useless one there just to stop the compiler barfing. See https://stackoverflow.com/questions/3065154/undefined-reference-to-vtable
// All subclasses will override this.   Note same issue on sensor_float and sensor_uint16 and sensor_ht
void Sensor_HT::readAndSet() { Serial.println("Sensor_HT::readAndSet must be subclassed"); }

#endif // SENSOR_HT_WANT