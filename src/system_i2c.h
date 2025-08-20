#ifndef SYSTEM_I2C_H
#define SYSTEM_I2C_H
#include <Arduino.h>
#include <Wire.h> // Allow things that include system_spi.h to use constants from Wire (if any)

class System_I2C {
  public:
    uint8_t addr;
    TwoWire* wire;
    System_I2C(uint8_t addr, TwoWire* wire = &Wire);
    void initialize();
    void send(uint8_t cmd);  
    bool send(uint8_t* buf, uint8_t bytes);  
    bool read(uint8_t* buf, uint8_t bytes);
    uint32_t read(uint8_t bytes); // bytes <= 4
    uint8_t send1read1(uint8_t cmd); // 1->1
    bool sendAndRead(uint8_t* sendBuffer, uint8_t sendLength, uint8_t* rcvBuffer,uint8_t rcvLength);  // N->M
    bool sendAndRead(uint8_t cmd, uint8_t* rcvBuffer,uint8_t rcvLength);  // 1->N
};
#endif // SYSTEM_I2C_H
