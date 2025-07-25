#ifndef SENSOR_MS5803_H
#define SENSOR_MS5803_H
/* Frugal IoT MS803 pressure sensor support
 * 
 * This is freestanding code since I was unable to find an existing library that matched requirements.
 * 
 * Some code is inspired from https://github.com/vic320/Arduino-MS5803-14BA which does SPI but 
 * does not appear to be supported any more, and is not available from the Platform IO platform.
 * 
 */

#include <Arduino.h>
// #include "system_spi.h" // Not currently working - needs revising to match system_i2c patterns
#include "system_i2c.h"
#include "sensor.h"

class Sensor_ms5803 : public Sensor {
  public:
    OUTfloat* pressure;  
    OUTfloat* temperature;
    // System_SPI interface; // SPI object // Not currently working - needs revising to match system_i2c patterns
    System_I2C interface; // I2C object
    uint16_t sensorCoefficients[8];
    //float press = 0;  // Stores actual pressure in mbars
    //float temp = 0;   // Stores actual temp in degrees C.

    uint8_t CRC4(uint16_t n_prom[]);
    Sensor_ms5803(const char* const id, const char * const name, uint8_t address);
    ~Sensor_ms5803(); //TODO-132
    void setup() override; 
    uint8_t ms5803CRC4();
    void readAndSet() override; // Override in Sensor
    void dispatchTwig(const String &topicSensorId, const String &leaf, const String &payload, bool isSet);
 };
 
 #endif // SENSOR_MS5803_H