/*
 * Sensor Battery
 * Read from some internal setup - that is board specific and report millivolts
 * 
 * On:
 * ARDUINO_LOLIN_C3_PICO there is a solder jump to pin 3
 * ARDUINO_LOLIN_C3_MINI there is no battery pin
 * ESP8266 D1 shields can only use A0 as it is the only analog pin, D1 shields use this
 * LilyGo HiGrow uses pin 33
 * 
 * Heltec is complex - see https://digitalconcepts.net.au/arduino/index.php?op=Battery#wifilora3 and note V3 and v3.2 are different
 *
 */

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "sensor_analog.h"
#include "sensor_battery.h"

// Note voltage divider is board specific - known defaults in sensor_battery.h

Sensor_Battery::Sensor_Battery(const uint8_t pin_init, float voltage_divider) 
//(id, name, pin, width, min, max, offset, scale, color, retain) 
: Sensor_Analog("battery", "Battery", pin_init, 0, 0, 4.5, 0, voltage_divider, "green", true) //TODO-1
  {
      pinMode(pin, INPUT); // Maybe not needed, but really need to be sure for power
   }

// ESP32 readInt() override - must be outside conditional block to avoid vtable errors
// when using the 2-parameter constructor on boards without default SENSOR_BATTERY_PIN/VOLTAGE_DIVIDER
#ifdef ESP32
int Sensor_Battery::readInt() {
  return analogReadMilliVolts(pin);  // returns uint32 
}
#endif // ESP32

#if defined(SENSOR_BATTERY_VOLTAGE_DIVIDER) && defined(SENSOR_BATTERY_PIN)
  #ifdef ESP8266 // analogReadMilliVolts not available

    #define ANALOG_READ_RANGE 1024 // This can be board/chip specific, 
    #define VCC_MILLIVOLTS 1000.0 // Voltage at chip pin at which we get ANALOG_READ_RANGE
    // Note that on some boards -  the voltage divider for the battery is different than for pin A0
    // e.g. ARDUINO_ESP8266_WEMOS_D1MINIPRO (V2) - batt = (130+220+100)/100 while A0 is just (220+100)/100

    #define SENSOR_BATTERY_SCALE (SENSOR_BATTERY_VOLTAGE_DIVIDER * VCC_MILLIVOLTS) / ANALOG_READ_RANGE 

    // readInt() will default to read this pin
    // convert() will apply scale
  #else  // ESP32
    #ifdef SENSOR_BATTERY_VOLTAGE_DIVIDER
      #define SENSOR_BATTERY_SCALE (SENSOR_BATTERY_VOLTAGE_DIVIDER)
    #endif
  #endif //ESP8266

#endif // SENSOR_BATTERY_VOLTAGE_DIVIDER && SENSOR_BATTERY_PIN
