#ifndef SENSOR_ANALOG_H
#define SENSOR_ANALOG_H

/* Configuration options.
 *  
 */

// Add new analog sensors to this statement.  #TO_ADD_SENSOR
#if defined(SENSOR_ANALOG_EXAMPLE_DEBUG) || defined(SENSOR_BATTERY_DEBUG) || defined(SENSOR_SOIL_DEBUG) // TODO make this generic, but LED almost always wanted
#define SENSOR_ANALOG_DEBUG
#endif 

class Sensor_Analog {
  public: 
    uint8_t pin;
    uint16_t value = 0;
    // If non-zero smoothed by this many bits (newSmoothedValue = oldSmoothedValue - (oldSmoothedValue>>smooth) + (reading))
    // Be careful of overflow - e.g. if 10 bit analog read then max smooth can be is 6 to smooth over 2^6 = 64 readings
    uint8_t smooth = 0; 
    unsigned long ms = 10000; // 10 second read
    char *topic;
    unsigned long nextLoopTime = 0;
    #ifdef SENSOR_ANALOG_DEBUG
      String *name;
    #endif
    Sensor_Analog(const uint8_t p);
    virtual void act(); // send results
    virtual void set(const uint16_t v); // convert from value read to value stored
    virtual uint16_t read();
    void setup();
    void loop();
}; // Class Actuator_Digital

#endif // SENSOR_ANALOG_H
