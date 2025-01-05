/*
  Base class for sensors that have a Uint16_t value
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <forward_list>
#include "sensor.h"
#include "sensor_uint16.h"
#include "system_mqtt.h"

#if defined(SENSOR_ANALOG_EXAMPLE_DEBUG) || defined(SENSOR_BATTERY_DEBUG) || defined(SENSOR_SOIL_DEBUG) // TODO make this generic, but LED almost always wanted
  #define SENSOR_DEBUG
#endif

//Sensor_Uint16::Sensor_Uint16() : Sensor() {  };
Sensor_Uint16::Sensor_Uint16(const uint8_t smooth_init, const char* topic_init, const unsigned long ms_init)
  : Sensor(topic_init, ms_init), smooth(smooth_init) {}
    

// TODO_C++_EXPERT this next line is a completely useless one there just to stop the compiler barfing. See https://stackoverflow.com/questions/3065154/undefined-reference-to-vtable
// All subclasses will override this.   Note same issue on sensor_float and sensor_uint16
uint16_t Sensor_Uint16::read() { Serial.println("Sensor_Uint16::read must be subclassed"); return -1; }

void Sensor_Uint16::set(uint16_t newvalue) {

  // TODO-25 can be copy/adapted to Sensor_Float if needed
  uint16_t vv;
  if (smooth) {
    vv = value - (value >> smooth) + newvalue;
  } else {
    vv = newvalue;
  }
  #ifdef SENSOR_DEBUG
    Serial.print(topic);
    if (smooth) { Serial.print(F(" Smoothed")); }
    Serial.print(" "); Serial.println(vv);
  #endif // SENSOR_DEBUG

  if (changed(newvalue)) {
    value = vv;
    act();
  }
}
bool Sensor_Uint16::changed(uint16_t newvalue) {
  return (newvalue != value);
}
void Sensor_Uint16::act() {
    if (topic) {
      xMqtt::messageSend(topic, value, retain, qos); // Note messageSend will convert value to String and expand topic
    }
}
void Sensor_Uint16::loop() {
  if (nextLoopTime <= millis()) {
    set(read()); // Will also send message via act()
    nextLoopTime = millis() + ms;
  }
}