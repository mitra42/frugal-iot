/* Frugal IoT System I2C 
 * Support for generic I2C interface
 * Note that some senors use I2C via other included libraries
 * 
 */
#include "_settings.h"
#include <Wire.h>
#include "system/i2c.h"

System_I2C::System_I2C(uint8_t addr, TwoWire* wire)
:  addr(addr), wire(wire) {}

// A System_I2C is per *device*, but wire->begin() is per *bus*, and every device on a bus calls
// initialize() from its own setup(). So remember which buses have been begun - see the
// "unnecessary since already called" note in ens160aht21.cpp and TODO-115/TODO-16 in sht.cpp.
static TwoWire* i2c_begun[SYSTEM_I2C_MAX_BUSES] = { nullptr };

// True if this bus has been begun before. If not, records it (when there is room) and returns
// false so the caller does the begin().
bool System_I2C::busAlreadyBegun() {
  bool found = false;
  uint8_t slot = SYSTEM_I2C_MAX_BUSES; // First free slot, if any
  for (uint8_t i = 0; i < SYSTEM_I2C_MAX_BUSES; i++) {
    if (i2c_begun[i] == wire) {
      found = true;
    } else if ((i2c_begun[i] == nullptr) && (slot == SYSTEM_I2C_MAX_BUSES)) {
      slot = i;
    }
  }
  if (!found && (slot < SYSTEM_I2C_MAX_BUSES)) {
    i2c_begun[slot] = wire;
  }
  return found;
}

void System_I2C::initialize() {
  if (!busAlreadyBegun()) {
    wire->begin(I2C_SDA, I2C_SCL);  // typically SDA SCL unless board specific in _settings.h or overridden in platformio.ini
  }
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
// Read buffer from I2C - arbitrary length
bool System_I2C::read(uint8_t* buf, uint8_t bytes) {
  wire->requestFrom(addr, bytes);
  for (uint8_t i = 0; i < bytes; i++) {
    buf[i] = wire->read(); // TODO allow for failure and return true or false.
  }
  #ifdef SYSTEM_I2C_DEBUG
    Serial.print(F("I2C read"));
    for (uint8_t i = 0; i < bytes; i++) {
      Serial.print(buf[i], HEX); Serial.print(F(" "));
    }
    Serial.println();
  #endif
  return true;
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
    Serial.print(F("I2C read"));  Serial.println(result);
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

// Does `address` ACK on this bus? The primitive behind both isPresent() and scan().
bool System_I2C::ack(uint8_t address) {
  wire->beginTransmission(address);
  return wire->endTransmission() == 0;
}

// Cheap check that something is wired at this device's address, before reading any
// chip-specific id register.
bool System_I2C::isPresent() {
  return ack(addr);
}

void System_I2C::scan() {
  // Print the actual GPIO numbers Wire is using so wiring can be verified.
  // If 5V power is used for the backpack, its pull-up resistors will drive
  // SDA/SCL to 5V — ESP32 GPIOs are NOT 5V-tolerant. Use 3.3V instead.
  Serial.print(F("Scanning I2C on SDA=")); Serial.print(I2C_SDA);
  Serial.print(F(" SCL=")); Serial.println(I2C_SCL);
  bool found = false;
  delay(1000); // TOOD-XXX remove this once sure what needed
  // Note this scans *this object's* bus - it used to scan the global I2C_WIRE regardless, so a
  // device constructed on Wire1 had its scan report the wrong bus entirely.
  for (uint8_t a = 1; a < 127; a++) {
    if (ack(a)) {
      Serial.print(F("  device at 0x")); Serial.println(a, HEX);
      found = true;
    }
  }
  if (!found) Serial.println(F("  nothing found - check wiring and that SDA/SCL are correct gpio numbers above"));
}