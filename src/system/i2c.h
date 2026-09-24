// Deep Sleep issues: none - the bus is re-initialised in setup(), and after a light sleep by System_Interface::initializeAll().
#ifndef SYSTEM_I2C_H
#define SYSTEM_I2C_H
#include <Arduino.h>
#include <Wire.h> // Allow things that include system_spi.h to use constants from Wire (if any)
#include "system/interface.h"

/* The pins that switch power to the I2C bus - the pull-ups as well as the chips on it.
 *
 * Applied to every bus as it is created, because the ordinary node has one switched rail and one
 * bus. A board with Wire and Wire1 on DIFFERENT rails should call powerPins() on whichever bus
 * is the exception, e.g. System_I2C_Bus::forWire(&Wire1)->powerPins(pin, PIN_NONE).
 */
#ifndef SYSTEM_I2C_POWER3v3_PIN
  #define SYSTEM_I2C_POWER3v3_PIN PIN_NONE
#endif
#ifndef SYSTEM_I2C_POWER0_PIN
  #define SYSTEM_I2C_POWER0_PIN PIN_NONE
#endif

/* One physical I2C bus - one TwoWire, its begin(), its power, and the scan.
 *
 * Split out from System_I2C because that is per DEVICE while all of this is per BUS: every
 * device on a bus calls initialize() from its own setup(), and only the first of them should
 * do the begin(). See the "unnecessary since already called" note in ens160aht21.cpp and
 * TODO-115/TODO-16 in sht.cpp for what that de-duplication is for.
 *
 * It is also what carries the bus's power pins, so that a sensor's powerPins() reaches the rail
 * that actually feeds it - see system/interface.h for the whole argument.
 */
class System_I2C_Bus : public System_Interface {
  public:
    // The bus for this TwoWire, created on first use. Always use this rather than constructing
    // one - two objects for one bus would each begin() it and each power it separately.
    static System_I2C_Bus* forWire(TwoWire* wire);
    void initialize() override; // begin(), once per bus and again after a power cycle
    // With SYSTEM_I2C_DEBUG, initialize() calls this once each time the bus comes up.
    void scan();
    bool ack(uint8_t address); // Does `address` ACK on this bus? The primitive behind scan()
    TwoWire* const wire;
  private:
    System_I2C_Bus(TwoWire* wire);
};

class System_I2C {
  public:
    uint8_t addr;
    TwoWire* wire;
    System_I2C(uint8_t addr, TwoWire* wire = &Wire);
    void initialize(); // Idempotent per bus - every device on a bus calls it from its own setup()
                       // With SYSTEM_I2C_DEBUG, scans the bus once, each time it comes up.
    /* The shared bus this device is on - what a sensor hands back from powerInterface(), and
     * where a sketch reaches a second bus's power pins.
     */
    System_I2C_Bus* bus() { return bus_; }
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
    System_I2C_Bus* bus_; // The shared bus - power, begin() and the scan all live there
};
#endif // SYSTEM_I2C_H
