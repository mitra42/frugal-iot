#ifndef SYSTEM_SPI_H
#define SYSTEM_SPI_H
#include <Arduino.h>
#include <SPI.h> // Allow things that include system_spi.h to use constants like SPI_CLOCK_DIV2

class System_SPI {
  public:
    uint8_t cs;
    uint64_t clock;
    System_SPI(uint8_t cs, uint64_t clock);
    void initialize();
    void send(uint8_t cmd);  
    uint32_t read(uint8_t cmd, uint8_t bytes);
    uint16_t read16(uint8_t cmd);
};
#endif // SYSTEM_SPI_H
