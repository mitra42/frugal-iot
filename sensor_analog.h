#ifndef SENSOR_ANALOG_H
#define SENSOR_ANALOG_H

/* Configuration options.
 * Optional SENSOR_xxx_DEBUG 
 */
 #include "sensor.h"
 #include "sensor_uint16.h"

// Add new analog sensors to this statement.  #TO_ADD_SENSOR
#if defined(SENSOR_ANALOG_EXAMPLE_DEBUG) || defined(SENSOR_BATTERY_DEBUG) || defined(SENSOR_SOIL_DEBUG) // TODO make this generic, but LED almost always wanted
  #define SENSOR_ANALOG_DEBUG
#endif 

class Sensor_Analog : public Sensor_Uint16 {
  public: 
    uint8_t pin;
    
    Sensor_Analog(const uint8_t p);
    Sensor_Analog(const uint8_t pin, const uint8_t smooth, const char* topic, const unsigned long ms);
    virtual void setup();
    virtual uint16_t read();
}; // Class Sensor_Analog
#endif // SENSOR_ANALOG_H
