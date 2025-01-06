#ifndef SENSOR_SOIL_H
#define SENSOR_SOIL_H

/* Configuration options
 * Required: SENSOR_SOIL_PIN
 * Optional: SENSOR_SOIL_MS SENSOR_SOIL_REFERENCE SENSOR_SOIL_SMOOTH
*/


#ifndef SENSOR_SOIL_TOPIC
  #define SENSOR_SOIL_TOPIC "soil"
#endif
#ifndef SENSOR_SOIL_NAME
  #define SENSOR_SOIL_NAME "Soil"
#include "sensor_analog.h"
#endif
#define SENSOR_SOIL_ADVERTISEMENT "\n  -\n    topic: " SENSOR_SOIL_TOPIC "\n    name: " SENSOR_SOIL_NAME "\n    type: int\n    display: bar\n    min: 0\n    max: 100\n    color: brown\n    rw: r"

class Sensor_Soil : public Sensor_Analog {
  public: 
    Sensor_Soil(const uint8_t p);
    virtual void act();
    virtual uint16_t read();
};

namespace sSoil {
void setup();
void loop();
} // sSoil

#endif // SENSOR_SOIL_H
