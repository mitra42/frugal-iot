/*
  Sensor Battery
  Read from some internal setup - that is board specific and report millivolts
*/

#include "_settings.h"  // Settings for what to include etc
#ifdef SENSOR_BATTERY_WANT
#include <Arduino.h>
#include "sensor_analog.h"
#include "sensor_battery.h"
#include "system_discovery.h"

#ifdef LOLIN_C3_PICO
  #define SENSOR_BATTERY_PIN 3
#else
  #error Measuring battery voltage is board specific, only currently defined for Lolin C3 Pico
#endif

Sensor_Battery::Sensor_Battery(const uint8_t p) : Sensor_Analog(p) { }

uint16_t Sensor_Battery::read() {
  return analogReadMilliVolts(pin); // Note this returns uiunt32_t which makes no sense given max value is 5*1000 = 5000
}

namespace sBattery {

Sensor_Battery sensor_battery(SENSOR_BATTERY_PIN);  // TODO-57 will rarely be as simple as this

void setup() {
  #ifdef SENSOR_BATTERY_DEBUG
    sensor_battery.name = new String(F("battery"));
  #endif // SENSOR_BATTERY_DEBUG
  sensor_battery.topic = String(*xDiscovery::topicPrefix + SENSOR_BATTERY_TOPIC);
  sensor_battery.setup();
}
// TODO create a linked list of sensors, and another of actuators that can be called in loop OR use list by time
void loop() {
  sensor_battery.loop();
}

} //namespace sBattery
#endif // SENSOR_BATTERY_WANT
