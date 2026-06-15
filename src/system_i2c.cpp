/* Frugal IoT System I2C 
 * Support for generic I2C interface
 * Note that some senors use I2C via other included libraries
 * 
 */
#include "_settings.h"
#include <Wire.h>
#include "system_i2c.h"

System_I2C::System_I2C(uint8_t addr, TwoWire* wire) 
:  addr(addr), wire(wire) {}

void System_I2C::initialize() {
  wire->begin(I2C_SDA, I2C_SCL);  // typically SDA SCL unless board specific in _settings.h or overridden in platformio.ini
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
    Serial.print(F()"I2C read"));  Serial.println(result); 
  #endif
  return result;
}

// Now various combinations used by different sensors - some will be in the sensor classes instead.
// TODO-101 rewrite ms803 to use this

// Send 1 byte, read N
uint8_t System_I2C::send1read1(uint8_t cmd) {
  send(cmd);
  return read(1);
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

void System_I2C::scan() {
  // Print the actual GPIO numbers Wire is using so wiring can be verified.
  // If 5V power is used for the backpack, its pull-up resistors will drive
  // SDA/SCL to 5V — ESP32 GPIOs are NOT 5V-tolerant. Use 3.3V instead.
  Serial.print(F("Scanning I2C on SDA=")); Serial.print(I2C_SDA);
  Serial.print(F(" SCL=")); Serial.println(I2C_SCL);
  bool found = false;
  delay(1000); // TOOD-XXX remove this once sure what needed
  for (uint8_t addr = 1; addr < 127; addr++) {
    I2C_WIRE.beginTransmission(addr);
    if (I2C_WIRE.endTransmission() == 0) {
      Serial.print(F("  device at 0x")); Serial.println(addr, HEX);
      found = true;
    }
  }
  if (!found) Serial.println(F("  nothing found - check wiring and that SDA/SCL are correct gpio numbers above"));
}