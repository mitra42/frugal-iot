#ifndef SYSTEM_I2C_H
#define SYSTEM_I2C_H
#include <Arduino.h>
#include <Wire.h> // Allow things that include system_spi.h to use constants from Wire (if any)

// How many distinct TwoWire buses initialize() will remember having begun. ESP32 has Wire and
// Wire1; if a board ever had more, exceeding this just means begin() gets called again, which
// is what happened everywhere before the de-duplication - i.e. safe, just not deduped.
#ifndef SYSTEM_I2C_MAX_BUSES
  #define SYSTEM_I2C_MAX_BUSES 2
#endif

class System_I2C {
  public:
    uint8_t addr;
    TwoWire* wire;
    System_I2C(uint8_t addr, TwoWire* wire = &Wire);
    void initialize(); // Idempotent per bus - every device on a bus calls it from its own setup()
    void send(uint8_t cmd);
    bool send(uint8_t* buf, uint8_t bytes);
    bool read(uint8_t* buf, uint8_t bytes);
    uint32_t read(uint8_t bytes); // bytes <= 4
    // Write one byte to a register - the register/value paradigm most chips use.
    bool sendRegister(uint8_t reg, uint8_t value);
    // Write a big-endian 16-bit value to a register. Counterpart to send1read(reg, 2).
    bool sendRegister16(uint8_t reg, uint16_t value);
    uint8_t send1read1(uint8_t cmd); // 1->1
    // Send a register/command byte then read `bytes` back as a big-endian integer. bytes <= 4.
    uint32_t send1read(uint8_t cmd, uint8_t bytes);
    bool sendAndRead(uint8_t* sendBuffer, uint8_t sendLength, uint8_t* rcvBuffer,uint8_t rcvLength);  // N->M
    bool sendAndRead(uint8_t cmd, uint8_t* rcvBuffer,uint8_t rcvLength);  // 1->N
    // True if anything ACKs at this device's address. Cheap wiring/address check, before any
    // chip-specific id register read.
    bool isPresent();
    void scan();
  protected:
    bool ack(uint8_t address); // Does `address` ACK on this bus?
    bool busAlreadyBegun();    // True if this bus was begun before; records it if not
};
#endif // SYSTEM_I2C_H
