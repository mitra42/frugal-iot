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
 */

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "sensor_analog.h"
#include "sensor_battery.h"

// TODO Currently, the offset and scale functionality of Sensor_Analog is not being used. 
// TODO msot of the functionality below could be replaced by setting a scale at setup()

// Note voltage divider is board specific - known defaults in sensor_battery.h
Sensor_Battery::Sensor_Battery(const uint8_t pin_init, const float voltage_divider) 
: Sensor_Analog("battery", "Battery", pin_init, 0, 0, 4.5, 0, 1000, "green", true),
  voltage_divider(voltage_divider)
  { }

#ifdef ESP8266 // analogReadMilliVolts not available

  #define ANALOG_READ_RANGE 1024 // THis can be board/chip specific, 
  #define VCC_MILLIVOLTS 1000.0 // Voltage at chip pin at which we get ANALOG_READ_RANGE
  // Note that on some boards -  the voltage divider for the battery is different than for pin A0
  // e.g. ARDUINO_ESP8266_WEMOS_D1MINIPRO (V2) - batt = (130+220+100)/100 while A0 is just (220+100)/100

  // Note the ESP32 function returns uint32_t which makes no sense given max battery is 5000
  uint16_t analogReadMilliVolts(const uint8_t pin) {
    /* THere may be a reason to do it this way to get float calcs ?
    const float VCC_Volt = 5.000; // ( 5v for 8bits Arduino boards, 3.3v for ESP, STM32 and SAMD )
    const analogReadRange = 1024;  // for Arduino boards
    uint16_t analogValue = analogRead(pin);
    uint16_t milliVolts = (VCC_Volt * 1000 * analogValue) /  analogReadRange;
    */
    static const float multiplier = VCC_MILLIVOLTS / ANALOG_READ_RANGE ; 
    const float analogValue = analogRead(pin);
    #ifdef SENSOR_BATTERY_DEBUG
      Serial.print(F("Battery read:")); Serial.print(analogValue); Serial.print(F(" multiplier "));  Serial.print(multiplier); Serial.print(F(" report ")); Serial.println(analogValue * multiplier); 
    #endif
    return analogValue * multiplier;  // Note this is millivolts at A0, which has been divided by voltage_divider
  }
#endif //ESP8266

float Sensor_Battery::read() {
  // Note - have tested this will do a float multiplication if voltage_divider is a float
    return analogReadMilliVolts(pin) * voltage_divider; 
}

