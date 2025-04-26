#ifndef SENSOR_ENSAHT_H
#define SENSOR_ENSAHT_H
#include "_settings.h"  // Settings for what to include etc

#ifdef SENSOR_ENSAHT_WANT
#include <Arduino.h>
#include "system_i2c.h"
#include "sensor.h"

class Sensor_ensaht : public Sensor {
  public:
    //OUTfloat* pressure;  
    //OUTfloat* temperature;
    System_I2C aht; // I2C object for AHT
    System_I2C ens; // I2C object for ENS

    Sensor_ensaht(const char* name);
    ~Sensor_ensaht; //TODO-101
    void setup(); 
    void readAndSet();
 };
 
#endif // SENSOR_ENSAHT_WANT
#endif // SENSOR_ENSAHT_H
