#ifndef SENSOR_SOIL_H
#define SENSOR_SOIL_H

/* Configuration options
 * Required: SENSOR_SOIL_PIN
 * Optional: SENSOR_SOIL_MS SENSOR_SOIL_REFERENCE SENSOR_SOIL_SMOOTH
*/

#include "sensor_analog.h"

#ifndef SENSOR_SOIL_TOPIC
  #define SENSOR_SOIL_TOPIC "soil"
#endif
#ifndef SENSOR_SOIL_NAME
  #define SENSOR_SOIL_NAME "Soil"
#endif

#ifndef SENSOR_SOIL_PIN
  //TO_ADD_BOARD
  #ifdef LOLIN_C3_PICO
    #error there is no default pin on C3-pico suggest 4, 0 or 1
  #else
    #ifdef ESP8266_D1
      #define SENSOR_SOIL_PIN A0 // Which pin to read - this will be board specific
    #else 
      #error Sorry no default Analog pin for your board
    #endif
  #endif
#endif

#ifndef SENSOR_SOIL_MS
  #define SENSOR_SOIL_MS 15000 // How often to read in MS
#endif

#ifndef SENSOR_SOIL_0
  #define SENSOR_SOIL_0 3000
#endif
#ifndef SENSOR_SOIL_100
  #define SENSOR_SOIL_100 1000
#endif

#define SENSOR_SOIL_ADVERTISEMENT1 "\n  -\n    topic: " SENSOR_SOIL_TOPIC "\n    name: " SENSOR_SOIL_NAME "\n    type: int\n    display: bar\n    min: 0\n    max: 100\n    color: brown\n    rw: r"
#define SENSOR_SOIL_ADVERTISEMENT2 "\n  -\n    topic: " SENSOR_SOIL_TOPIC "2\n    name: " SENSOR_SOIL_NAME "2\n    type: int\n    display: bar\n    min: 0\n    max: 100\n    color: brown\n    rw: r"
#define SENSOR_SOIL_ADVERTISEMENT3 "\n  -\n    topic: " SENSOR_SOIL_TOPIC "3\n    name: " SENSOR_SOIL_NAME "3\n    type: int\n    display: bar\n    min: 0\n    max: 100\n    color: brown\n    rw: r"

class Sensor_Soil : public Sensor_Analog {
  public: 
    uint16_t map0;
    uint16_t map100;
    Sensor_Soil(const uint16_t map0, const uint16_t map100, const uint8_t pin_init, const uint8_t smooth_init, const char* topic_init, const unsigned long ms_init);
    virtual bool changed(uint16_t newvalue);
    virtual uint16_t read();
};

#endif // SENSOR_SOIL_H
