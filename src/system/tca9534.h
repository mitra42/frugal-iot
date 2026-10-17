// Deep Sleep issues: none of its own - the chip keeps its outputs through any ESP32 sleep PROVIDED its supply does. See below.
/* Frugal-IoT - System_TCA9534 - an 8 bit I2C port expander, shared by everything on its pins
 *
 * Also drives the register-compatible PCA9534, TCA9554/PCA9554 and TCA6408: four 8 bit registers,
 *   0 input   1 output   2 polarity (inputs only)   3 configuration (1 = input, the power-on state)
 * Addresses: TCA9534 / PCA9534 0x20-0x27; the "A" parts and PCA9554A 0x38-0x3F.
 *
 * One object per CHIP, not per pin - forAddress() returns the same object to every caller. Several
 * outputs on one chip all write the same output register, so each has to change only its own bit
 * and leave the others as they are. A shadow of that register, held here, is what lets them;
 * reading it back before every write would do the same at twice the I2C traffic.
 *
 * Why this is not just a GPIO number
 * ----------------------------------
 * The other obvious design is a pin-number convention - "pins 100-107 are the expander" - with
 * digitalWrite() wrapped. It was rejected because the expander needs things a GPIO does not: a bus
 * to be begun and powered, a chip that may not answer, and a power-on state to be handled (below).
 * A GPIO number has nowhere to keep any of that.
 *
 * Power-on, sleep, and the one thing to get right
 * -----------------------------------------------
 * At power-on the output register reads 0xFF and every pin is an INPUT. So "read the output
 * register, then make the pin an output" turns every output ON for a moment on a cold boot - on a
 * relay board, every valve. configureOutput() avoids that by looking at the configuration register
 * first: a pin still configured as an input means the chip has been through a power cycle, and is
 * given the caller's initial value BEFORE it becomes an output. A pin that is already an output
 * means the chip has kept its state - a warm reboot, or a wake from deep sleep - and is left alone.
 *
 * That is also how this survives a deep sleep with no RTC memory and no gpio_hold: the chip IS the
 * memory, as long as it stays powered. Put it on a rail that the node does not switch off - its I2C
 * pull-ups can be on a switched rail, the chip itself must not be. If it is switched off with the
 * rest, every output on it drops at the sleep, which preserveDuringSleep() cannot prevent.
 *
 * Inputs (a button on an expander pin, say) would sit on this same object - nothing here assumes
 * a pin is an output until configureOutput() says so.
 */
#ifndef SYSTEM_TCA9534_H
#define SYSTEM_TCA9534_H

#include <Arduino.h>
#include "system/i2c.h"

#ifndef SYSTEM_TCA9534_ADDRESS
  #define SYSTEM_TCA9534_ADDRESS 0x20 // A0-A2 all low
#endif

class System_TCA9534 {
  public:
    // The chip at this address on this bus, created on first use. Always use this rather than
    // constructing one - two objects for one chip would each keep a shadow, and overwrite each other.
    static System_TCA9534* forAddress(uint8_t addr = SYSTEM_TCA9534_ADDRESS, TwoWire* wire = &I2C_WIRE);
    void initialize(); // The bus - idempotent, so every pin's owner can call it
    /* Make `pin` (0-7) an output. If the chip has been through a power cycle since it was last configured,
     * it is driven to `initial` (a raw level, already inverted if need be) BEFORE it becomes an
     * output; otherwise whatever it holds now is kept. False if the chip did not answer.
     */
    bool configureOutput(uint8_t pin, bool initial);
    bool writeBit(uint8_t pin, bool level); // Raw level on the pin. False if the chip did not answer.
    System_I2C interface;
  private:
    System_TCA9534(uint8_t addr, TwoWire* wire);
    bool readRegister(uint8_t reg, uint8_t* value);
    uint8_t output = 0xFF; // Shadow of register 1. 0xFF is the chip's own power-on value
    bool present = false;  // Answered at the last configureOutput() - saves a failed write per act()
};

#endif // SYSTEM_TCA9534_H
