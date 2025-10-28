/*
 * Sensor Battery
 * Read from some internal setup - that is board specific and report millivolts
 * 
 * On:
 * ARDUINO_LOLIN_C3_PICO there is a solder jump to pin 3
 * ARDUINO_LOLIN_C3_MINI there is no battery pin
 * ESP8266 D1 shields can only use A0 as it is the only analog pin, D1 shields use this
 * LilyGo HiGrow uses pin 33
 * Lilygo T3S3 is on GPIO4 (IO04) (from google, not in pins.arduino.h)
 * 
 * Note that BAT_VOLT is defined in /Users/mitra/.platformio/packages/framework-arduinoespressif32/variants/lilygo_t3_s3_sx1262/pins_arduino.h and maybe useful as a semi-standard
 * grep BAT_VOLT /Users/mitra/.platformio/packages/framework-arduinoespressif32/variants/*./pins_arduino.h shows VBAT_VOLTAGE BAT_VOLTS or BAT_VOLT BAT_MEASURE VBAT_SENSE BATT_ADC_PIN BAT_ADC_PIN BAT_ADC BATT_MONITOR BAT_MEASURE BAT_LV on different boards
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
#elif defined(ARDUINO_LILYGO_T3_S3_V1_X)
  #define SENSOR_BATTERY_VOLTAGE_DIVIDER 2 // Almost certainly wrong ! Define for this board TO-ADD-BOARD
#else 
  #define SENSOR_BATTERY_VOLTAGE_DIVIDER 1 // Almost certainly wrong ! Define for this board TO-ADD-BOARD
#endif 

#ifndef SENSOR_BATTERY_PIN
  #ifdef ARDUINO_LILYGO_T3_S3_V1_X
  //#define SENSOR_BATTERY_PIN BAT_VOLT // a static const uint8_t
   #define SENSOR_BATTERY_PIN BAT_VOLT_PIN
  // TO-ADD-BOARD
  // Feel free to add #defines for whatever board you are using 
  // - look in e.g. ~/.platformio/platforms/espressif32/boards/lilygo-t3-s3.json for the board name define and variant file name
  // - look in ~/.platformio/packages/framework-arduinoespressif32/variants/lilygo_t3_s3_sx127x/pins_arduino.h for which way it defines the battery pin
  //#elif defined(XXX)
  // #define SENSOR_BATTER_PIN VBATT 
  #else 
    #define SENSOR_BATTERY_PIN 255 // Wont be on this pin, but allows code to compile on unknown board
  #endif
  // See https://github.com/espressif/arduino-esp32/issues/11953 for suggestion to fix this problem
#endif

class Sensor_Battery : public Sensor_Analog {
  public: 
    Sensor_Battery(const uint8_t pin = SENSOR_BATTERY_PIN);
  protected:
    #ifdef ESP32
      int readInt() override;
    #endif
};

#endif // SENSOR_BATTERY_H
