#ifndef SENSOR_ANALOG_H
#define SENSOR_ANALOG_H

/* Configuration options.
 * Optional SENSOR_xxx_DEBUG 
 */
 #include "sensor.h"
 #include "sensor_uint16.h"

class Sensor_Analog : public Sensor_Uint16 {
  public: 
    uint8_t pin;
    
    //Sensor_Analog(const uint8_t p);
    Sensor_Analog(const char* const id, const char * const name, const uint8_t pin, const uint8_t smooth, const uint16_t min, const uint16_t max, const char* color, const unsigned long ms, bool retain);
    virtual void setup();
    virtual uint16_t read();
}; // Class Sensor_Analog
#endif // SENSOR_ANALOG_H
