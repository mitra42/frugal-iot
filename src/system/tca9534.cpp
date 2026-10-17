/* Frugal-IoT - System_TCA9534 - see system/tca9534.h for the chip and why it is done this way. */

#include "_settings.h"
#include <vector>
#include "system/tca9534.h"

#define TCA9534_REG_OUTPUT 0x01
#define TCA9534_REG_CONFIG 0x03 // 1 = input, 0 = output

// One per chip for the lifetime of the process. Function-local, as i2c_buses() in i2c.cpp, so it
// exists however early the first actuator is constructed.
static std::vector<System_TCA9534*>& tca9534s() {
  static std::vector<System_TCA9534*> chips;
  return chips;
}

System_TCA9534* System_TCA9534::forAddress(uint8_t addr, TwoWire* wire) {
  System_TCA9534* found = nullptr;
  for (System_TCA9534* c : tca9534s()) {
    if (c->interface.addr == addr && c->interface.wire == wire) {
      found = c;
    }
  }
  if (!found) {
    found = new System_TCA9534(addr, wire);
    tca9534s().push_back(found);
  }
  return found;
}

System_TCA9534::System_TCA9534(uint8_t addr, TwoWire* wire)
: interface(addr, wire) {}

void System_TCA9534::initialize() {
  interface.initialize();
}

bool System_TCA9534::readRegister(uint8_t reg, uint8_t* value) {
  return interface.sendAndRead(reg, value, 1);
}

bool System_TCA9534::configureOutput(uint8_t pin, bool initial) {
  uint8_t config;
  uint8_t out;
  /* Both registers fresh from the chip, every time - not just the first. Another pin's
   * configureOutput() may have run since, and this is setup(), so the cost does not matter.
   */
  present = readRegister(TCA9534_REG_CONFIG, &config) && readRegister(TCA9534_REG_OUTPUT, &out);
  if (present) {
    output = out;
    if (config & (1 << pin)) {
      // Still an input, so the chip has been powered up since we last set it. Its output bit
      // is the 0xFF power-on value, not anything we chose - set it BEFORE it starts driving.
      present = writeBit(pin, initial)
        && interface.sendRegister(TCA9534_REG_CONFIG, config & ~(1 << pin));
    }
    // else already an output: the chip kept its state through our reboot or sleep - leave it be
  }
  if (!present) {
    Serial.printf("TCA9534 at 0x%02X did not answer - check the address and that it is powered\n", interface.addr);
  }
  return present;
}

bool System_TCA9534::writeBit(uint8_t pin, bool level) {
  bool ok = false;
  if (present) {
    if (level) {
      output |= (1 << pin);
    } else {
      output &= ~(1 << pin);
    }
    ok = interface.sendRegister(TCA9534_REG_OUTPUT, output);
  }
  #ifdef SYSTEM_TCA9534_DEBUG
    Serial.printf("TCA9534 0x%02X pin %u -> %u, output now 0x%02X%s\n", interface.addr, pin, level, output, ok ? "" : " NOT SENT");
  #endif
  return ok;
}
