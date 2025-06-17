#ifndef SENSOR_BATTERY_H
#define SENSOR_BATTERY_H

/* Configuration options
 * Required: 
 * Optional: SENSOR_BATTERY_PIN SENSOR_BATTERY_MS SENSOR_BATTERY_TOPIC
 */
// TODO-141 could use a rewrite - eg SENSOR_BATTERY_TOPIC is out of date

#include "sensor_analog.h"

#ifndef SENSOR_BATTERY_PIN
  #ifdef LOLIN_C3_PICO
    #define SENSOR_BATTERY_PIN (3) // There is a solder jump to pin 3 - which - on D1 shields - is same as A0 on ESP8266
  #elif defined(ESP8266)
    #define SENSOR_BATTERY_PIN A0 // Its the only analog pin so got to be here
  #elif defined(LILYGOHIGROW)
    #define SENSOR_BATTERY_PIN (33)
  #else
    #error Measuring battery voltage is board specific, only currently defined for a few boards
  #endif
#endif

#ifndef SENSOR_BATTERY_TOPIC
  #define SENSOR_BATTERY_TOPIC "battery"
#endif
#ifndef SENSOR_BATTERY_MS
  #define SENSOR_BATTERY_MS (60000)
#endif

class Sensor_Battery : public Sensor_Analog {
  public: 
    Sensor_Battery(const uint8_t pin);
    virtual uint16_t read();
};

#endif // SENSOR_BATTERY_H
