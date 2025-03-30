#ifndef SENSOR_SPI_H
#define SENSOR_SPI_H
#include <Arduino.h>
#include <SPI.h> // Allow things that include sensor_spi.h to use constants like SPI_CLOCK_DIV2
#include "sensor.h"

class Sensor_spi : public Sensor {
  public:
    uint8_t cs;
    uint64_t clock;
    Sensor_spi(const char* name, uint8_t cs, uint64_t clock, const unsigned long ms, bool retain);
    void initialize();
    void send(uint8_t cmd);  
    uint32_t read(uint8_t cmd, uint8_t bytes);
    uint16_t read16(uint8_t cmd);
};
#endif // SENSOR_SPI_H
