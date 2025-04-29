#ifndef SYSTEM_I2C_H
#define SYSTEM_I2C_H
#include <Arduino.h>
#include <Wire.h> // Allow things that include system_spi.h to use constants from Wire (if any)

class System_I2C {
  public:
    uint8_t addr;
    System_I2C(uint8_t addr);
    void initialize();
    void send(uint8_t cmd);  
    uint32_t read(uint8_t cmd, uint8_t bytes);
    uint16_t read16(uint8_t cmd);
};
#endif // SYSTEM_I2C_H
