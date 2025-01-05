/*
 * Sensor Battery
 * Read from some internal setup - that is board specific and report millivolts
 * 
 * Configuration options
 * Required: 
 * Optional: SENSOR_BATTERY_PIN SENSOR_BATTERY_MS SENSOR_BATTERY_TOPIC

*/

#include "_settings.h"  // Settings for what to include etc
#ifdef SENSOR_BATTERY_WANT
#include <Arduino.h>
#include "sensor_analog.h"
#include "sensor_battery.h"

Sensor_Battery::Sensor_Battery(const uint8_t pin_init) : Sensor_Analog(pin_init, 0, SENSOR_BATTERY_TOPIC, SENSOR_BATTERY_MS) { }

uint16_t Sensor_Battery::read() {
  // Shifted left two because reading is from divide by 2 resistors
  return analogReadMilliVolts(pin) << 1; // Note this returns uiunt32_t which makes no sense given max value is 5*1000 = 5000
}

#endif // SENSOR_BATTERY_WANT
