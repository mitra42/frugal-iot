#ifndef SENSOR_BATTERY_H
#define SENSOR_BATTERY_H

/* Configuration options
 * Required: 
 * Optional: SENSOR_BATTERY_PIN SENSOR_BATTERY_MS SENSOR_BATTERY_TOPIC
 */

#include "sensor_analog.h"

#ifdef LOLIN_C3_PICO
  #define SENSOR_BATTERY_PIN 3
#else
  #error Measuring battery voltage is board specific, only currently defined for Lolin C3 Pico
#endif

#ifndef SENSOR_BATTERY_TOPIC
  #define SENSOR_BATTERY_TOPIC "battery"
#endif
#ifndef SENSOR_BATTERY_MS
  #define SENSOR_BATTERY_MS 60000
#endif

#define SENSOR_BATTERY_ADVERTISEMENT "\n  -\n    topic: " SENSOR_BATTERY_TOPIC "\n    name: Battery\n    type: int\n    display: bar\n    min: 3000\n    max: 5000\n    color: green\n    rw: r"

class Sensor_Battery : public Sensor_Analog {
  public: 
    Sensor_Battery(const uint8_t pin);
    virtual uint16_t read();
};

#endif // SENSOR_BATTERY_H
