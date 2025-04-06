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

 #ifdef SENSOR_MS5803_WANT
 #include <Arduino.h>
 #ifdef SENSOR_MS5803_SPI
   #include "system_spi.h"
#elif defined(SENSOR_MS5803_I2C)
   #include "system_i2c.h"
#endif
 #include "sensor.h"

 class Sensor_ms5803 : public Sensor {
  public:
    OUTfloat* pressure;  
    OUTfloat* temperature;
    #ifdef SENSOR_MS5803_SPI
      System_SPI interface; // SPI object
    #elif defined(SENSOR_MS5803_I2C)
      System_I2C interface; // I2C object
    #endif
    uint16_t sensorCoefficients[8];
    //float press = 0;  // Stores actual pressure in mbars
    //float temp = 0;   // Stores actual temp in degrees C.

    uint8_t CRC4(uint16_t n_prom[]);
    Sensor_ms5803(const char* name);
    ~Sensor_ms5803(); //TODO-132
    void setup(); 
    uint8_t ms5803CRC4();
    void readAndSet(); // Override in Sensor
 };
 
 
 #endif // SENSOR_MS5803_WANT
 #endif