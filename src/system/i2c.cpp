/* Frugal IoT System I2C 
 * Support for generic I2C interface
 * Note that some senors use I2C via other included libraries
 * 
 */
#include "_settings.h"
#include <Wire.h>
#include <vector>
#include "system/i2c.h"

// ==================== System_I2C_Bus - one object per physical bus ====================

/* One bus per TwoWire, for the lifetime of the process. A std::vector rather than a map: a board
 * has one or two I2C buses, so a linear scan is smaller and faster than anything cleverer. Same
 * shape as System_OneWire::forPin().
 *
 * Function-local rather than a file static because a sensor holding a System_I2C by value can be
 * constructed during static initialization, and this is reached from that constructor.
 */
static std::vector<System_I2C_Bus*>& i2c_buses() {
  static std::vector<System_I2C_Bus*> buses;
  return buses;
}

System_I2C_Bus* System_I2C_Bus::forWire(TwoWire* wire) {
  for (System_I2C_Bus* b : i2c_buses()) {
    if (b->wire == wire) {
      return b;
    }
  }
  System_I2C_Bus* b = new System_I2C_Bus(wire);
  i2c_buses().push_back(b);
  return b;
}

System_I2C_Bus::System_I2C_Bus(TwoWire* wire)
: wire(wire) {
  // The ordinary node has one switched rail and one bus - see the note in i2c.h for the board
  // where that is not true.
  powerPins(SYSTEM_I2C_POWER3v3_PIN, SYSTEM_I2C_POWER0_PIN);
}

/* begin() the bus - once, and again after it has been through a power cycle.
 *
 * powerUp() first, so that a bus reached outside the System_Power lifecycle (a sketch talking to
 * a chip in its own setup(), say) still gets power rather than silently finding nothing. It is a
 * no-op when the rail is already up, which is the usual case.
 */
void System_I2C_Bus::initialize() {
  if (!initialized) {
    initialized = true;
    powerUp();
    wire->begin(I2C_SDA, I2C_SCL);  // typically SDA SCL unless board specific in _settings.h or overridden in platformio.ini
    #ifdef SYSTEM_I2C_DEBUG
      // Once per bus, from whichever device on it happens to call initialize() first - the
      // per-bus guard above is what makes this once-per-Wire rather than once-per-device.
      // Runs before any chip-specific configuration, so what it prints is the bus as wired.
      scan();
    #endif
  }
}

// Does `address` ACK on this bus? The primitive behind both System_I2C::isPresent() and scan().
bool System_I2C_Bus::ack(uint8_t address) {
  wire->beginTransmission(address);
  return wire->endTransmission() == 0;
}

void System_I2C_Bus::scan() {
  // Print the actual GPIO numbers Wire is using so wiring can be verified.
  // If 5V power is used for the backpack, its pull-up resistors will drive
  // SDA/SCL to 5V — ESP32 GPIOs are NOT 5V-tolerant. Use 3.3V instead.
  Serial.print(F("Scanning I2C on SDA=")); Serial.print(I2C_SDA);
  Serial.print(F(" SCL=")); Serial.println(I2C_SCL);
  bool found = false;
  delay(1000); // TOOD-XXX remove this once sure what needed
  // Note this scans *this* bus - it used to scan the global I2C_WIRE regardless, so a
  // device constructed on Wire1 had its scan report the wrong bus entirely.
  for (uint8_t a = 1; a < 127; a++) {
    if (ack(a)) {
      Serial.print(F("  device at 0x")); Serial.println(a, HEX);
      found = true;
    }
  }
  if (!found) Serial.println(F("  nothing found - check wiring and that SDA/SCL are correct gpio numbers above"));
}

// ==================== System_I2C - one per device on a bus ====================

System_I2C::System_I2C(uint8_t addr, TwoWire* wire)
:  addr(addr), wire(wire), bus_(System_I2C_Bus::forWire(wire)) {}

void System_I2C::initialize() {
  bus_->initialize();
}

// The raw send and write. 
// Send a single byte
  void System_I2C::send(uint8_t cmd) {
  // TODO-101 check for failure in write
  wire->beginTransmission(addr);
  wire->write(cmd);
  wire->endTransmission();
}
// Send buffer to I2C - arbitrary length
bool System_I2C::send(uint8_t* buf, uint8_t bytes) {
  // TODO-101 check for failure in write
  wire->beginTransmission(addr);
  for (uint8_t i = 0; i < bytes; i++) {
    if (wire->write(buf[i]) != 1) {
      return false;
    }
  }
  wire->endTransmission(); //TODO-101 want to return this value, but need to check others dont rely on inverse (1 = success)
  return true;
}
/* Read buffer from I2C - arbitrary length. False if the device did not supply them.
 *
 * TODO-101, now done for this one: this used to return true unconditionally, and on a device
 * that NACKed the read every byte came back as wire->read()'s -1, i.e. a buffer of 0xFF that
 * the caller had no way to tell from real data. A sensor that signals "not ready yet" by
 * NACKing the read - which is what both SHT families do with clock stretching disabled -
 * cannot be driven at all without this.
 */
bool System_I2C::read(uint8_t* buf, uint8_t bytes) {
  bool ok = (wire->requestFrom(addr, bytes) == bytes);
  if (ok) {
    for (uint8_t i = 0; i < bytes; i++) {
      buf[i] = wire->read();
    }
  }
  #ifdef SYSTEM_I2C_DEBUG
    if (ok) {
      Serial.print(F("I2C read "));
      for (uint8_t i = 0; i < bytes; i++) {
        Serial.print(buf[i], HEX); Serial.print(F(" "));
      }
      Serial.println();
    } else {
      Serial.printf("I2C read of %u bytes from 0x%02X got nothing\n", bytes, addr);
    }
  #endif
  return ok;
}
// Read from I2C - up to 4 bytes into a uint32_t
uint32_t System_I2C::read(uint8_t bytes) {
  wire->requestFrom(addr, bytes);
  uint32_t result = 0;
  for (uint8_t i = 0; i < bytes; i++) {
    result = result << 8;
    result |= wire->read();
  }
  #ifdef SYSTEM_I2C_DEBUG
    Serial.printf("I2C read %x\n",result);
  #endif
  return result;
}

// Write one byte to a register. The register/value paradigm most chips use - was hand-rolled
// identically in the ENS160's sendAndWait() and the BME280's writeReg() before this existed.
bool System_I2C::sendRegister(uint8_t reg, uint8_t value) {
  uint8_t buf[2] = { reg, value };
  return send(buf, 2);
}

// Write a big-endian 16-bit value to a register - chips whose registers are 16 bit wide,
// e.g. the INA219. Counterpart to send1read(reg, 2).
bool System_I2C::sendRegister16(uint8_t reg, uint16_t value) {
  uint8_t buf[3] = { reg, (uint8_t)(value >> 8), (uint8_t)(value & 0xFF) };
  return send(buf, 3);
}

// Now various combinations used by different sensors - some will be in the sensor classes instead.

// Send a register/command byte, read `bytes` back as a big-endian integer
uint32_t System_I2C::send1read(uint8_t cmd, uint8_t bytes) {
  send(cmd);
  return read(bytes);
}
// Send 1 byte, read 1
uint8_t System_I2C::send1read1(uint8_t cmd) {
  return send1read(cmd, 1);
}
// Send N bytes, read M
bool System_I2C::sendAndRead(uint8_t* sendBuffer, uint8_t sendLength, uint8_t* rcvBuffer,uint8_t rcvLength) {
  send(sendBuffer, sendLength); // TODO allow for failure here - if fails dont try the read just return false
  return read(rcvBuffer, rcvLength);
}
// Send 1 byte, read N
bool System_I2C::sendAndRead(uint8_t cmd, uint8_t* rcvBuffer,uint8_t rcvLength) {
  send(cmd); // TODO allow for failure here - if fails dont try the read just return false
  return read(rcvBuffer, rcvLength);
}

// Cheap check that something is wired at this device's address, before reading any
// chip-specific id register.
bool System_I2C::isPresent() {
  return bus_->ack(addr);
}

void System_I2C::scan() {
  bus_->scan();
}
