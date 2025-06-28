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

 #ifndef SENSOR_BATTERY_H
#define SENSOR_BATTERY_H

#include "sensor_analog.h"

 // Defines defaults - can use SENSOR_BATTERY_VOLTAGE_DIVIDER in main.cpp
#ifdef ARDUINO_ESP8266_WEMOS_D1MINIPRO // Note only works on D1 mini pro V2 (the Green one)
  #define SENSOR_BATTERY_VOLTAGE_DIVIDER 4.5 // (130+220+100)/100 i.e. 1V on A0 when 4.5 on batt 
#elif defined(ARDUINO_LOLIN_C3_PICO)
  #define SENSOR_BATTERY_VOLTAGE_DIVIDER 2 // Maybe board specific but most I see have 2 equal resistors
#elif defined(LILYGOHIGROW)
  #define SENSOR_BATTERY_VOLTAGE_DIVIDER 6.6 // From LilyGo code, not testd yet
#endif 


class Sensor_Battery : public Sensor_Analog {
  public: 
    float voltage_divider;
    Sensor_Battery(const uint8_t pin, float voltage_divider);
    virtual uint16_t read();
};

#endif // SENSOR_BATTERY_H
