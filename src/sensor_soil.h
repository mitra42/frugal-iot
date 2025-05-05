#ifndef SENSOR_SOIL_H
#define SENSOR_SOIL_H

/* Configuration options
 * Required: SENSOR_SOIL_PIN
 * Optional: SENSOR_SOIL_MS SENSOR_SOIL_REFERENCE SENSOR_SOIL_SMOOTH
*/

#include "sensor_analog.h"

#ifndef SENSOR_SOIL_PIN
  //TO_ADD_BOARD
  #if defined(LOLIN_C3_PICO)
    #error there is no default pin on C3-pico suggest 4, 0 or 1
  #elif defined(ESP8266_D1)
    #define SENSOR_SOIL_PIN A0 // Which pin to read - this will be board specific
  #elif defined(LILYGOHIGROW)
    #define SENSOR_SOIL_PIN 32
  #else 
    #error Sorry no default Analog pin for your board
  #endif
#endif


#ifndef SENSOR_SOIL_MS
  #define SENSOR_SOIL_MS 15000 // How often to read in MS
#endif

#ifndef SENSOR_SOIL_0
  #if defined(LILYGOHIGROW)
    #define SENSOR_SOIL_0 4095
  #else
    #define SENSOR_SOIL_0 3000
  #endif
#endif
#ifndef SENSOR_SOIL_100
  #if defined(LILYGOHIGROW)
    #define SENSOR_SOIL_100 0
  #else
    #define SENSOR_SOIL_100 1000
  #endif
#endif

#ifndef SENSOR_ANALOG_COLOR_1
  #define SENSOR_ANALOG_COLOR_1 "0x87643"
#endif



class Sensor_Soil : public Sensor_Analog {
  public: 
    uint16_t map0;
    uint16_t map100;
    Sensor_Soil(const char* const id, const char * const name, const uint16_t map0, const uint16_t map100, const uint8_t pin_init, const uint8_t smooth_init, const char* color, const unsigned long ms_init, bool retain);
    virtual uint16_t read();
    bool valid(uint16_t newvalue);
};

#endif // SENSOR_SOIL_H
