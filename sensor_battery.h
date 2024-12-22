#ifndef SENSOR_BATTERY_H
#define SENSOR_BATTERY_H

/* Configuration options
 * Required: - normally set based on board
 * Optional: SENSOR_BATTERY_PIN
*/


#include "sensor_analog.h"

#ifndef SENSOR_BATTERY_TOPIC
  #define SENSOR_BATTERY_TOPIC "battery"
#endif
#define SENSOR_BATTERY_ADVERTISEMENT "\n  -\n    topic: " SENSOR_BATTERY_TOPIC "\n    name: Battery\n    type: int\n    display: bar\n    min: 3000\n    max: 5000\n    color: green\n    rw: r"

class Sensor_Battery : public Sensor_Analog {
  public: 
    Sensor_Battery(const uint8_t p);
    virtual uint16_t read();
};

namespace sBattery {
void setup();
void loop();
} // sBattery

#endif // SENSOR_BATTERY_H
