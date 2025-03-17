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

Sensor_Battery::Sensor_Battery(const uint8_t pin_init) : Sensor_Analog(pin_init, 0, SENSOR_BATTERY_TOPIC, SENSOR_BATTERY_MS, true) { }

#ifdef ESP8266_D1_MINI_PROv2 // Green D1 pro
  #define VOLTAGE_DIVIDER 4.5 // (130+220+100)/100 i.e. 1V on A0 when 4.5 on batt 
#elif defined(LOLIN_C3_PICO)
  #define VOLTAGE_DIVIDER 2 // Maybe board specific but most I see have 2 equal resistors
#elif defined(LILYGOHIGROW)
  #define VOLTAGE_DIVIDER 6.6 // From LilyGo code, not testd yet
#else
  #error Voltage dividers are board specific - look at your schematic
#endif 

#ifdef ESP8266 // analogReadMilliVolts not available

  #define ANALOG_READ_RANGE 1024 // THis can be board/chip specific, 
  #define VCC_MILLIVOLTS 1000.0 // Voltage at chip pin at which we get ANALOG_READ_RANGE
  // Note that on some boards -  the voltage divider for the battery is different than for pin A0
  // e.g. ESP8266_D1_MINI_PROv2 - batt = (130+220+100)/100 while A0 is just (220+100)/100

  // Note the ESP32 function returns uint32_t which makes no sense given max battery is 5000
  uint16_t analogReadMilliVolts(uint8_t pin) {
    /* THere may be a reason to do it this way to get float calcs ?
    const float VCC_Volt = 5.000; // ( 5v for 8bits Arduino boards, 3.3v for ESP, STM32 and SAMD )
    const analogReadRange = 1024;  // for Arduino boards
    uint16_t analogValue = analogRead(pin);
    uint16_t milliVolts = (VCC_Volt * 1000 * analogValue) /  analogReadRange;
    */
    static const float multiplier = VCC_MILLIVOLTS / ANALOG_READ_RANGE ; 
    float analogValue = analogRead(pin);
    #ifdef SENSOR_BATTERY_DEBUG
      Serial.print("Battery read:"); Serial.print(analogValue); Serial.print(" multiplier ");  Serial.print(multiplier); Serial.print(" report "); Serial.println(analogValue * multiplier); 
    #endif
    return analogValue * multiplier;  // Note this is millivolts at A0, which has been divided by VOLTAGE_DIVIDER
  }
#endif //ESP8266

uint16_t Sensor_Battery::read() {
  // Note - have tested this will do a float multiplication if VOLTAGE_DIVIDER is a float
    return analogReadMilliVolts(pin) * VOLTAGE_DIVIDER; // Note this returns uiunt32_t which makes no sense given max value is 5*1000 = 5000
}

#endif // SENSOR_BATTERY_WANT
