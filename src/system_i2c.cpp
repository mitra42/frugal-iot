/* Frugal IoT System I2C 
 * Support for generic I2C interface
 * Note that some senors use I2C via other included libraries
 * 
 */
#include "_settings.h"
#ifdef SYSTEM_I2C_WANT
#include <Wire.h>
#include "system_i2c.h"

System_I2C::System_I2C(uint8_t addr) : addr(addr) {}

// Set a mask indicating when the device is busy - reads will wait for this to clear
System_I2C::busyWhen(uint8_t status) {
  busyWhen = status;
}
void System_I2C::initialize() {
  Wire.begin(); 
}
void System_I2C::send8(uint8_t cmd) {
  // TODO-101 check for failure in write
  Wire.beginTransmission(addr);
  Wire.write(cmd);
  Wire.endTransmission();
}
void System_I2C::send(uint8_t &buf, bytes) {
  // TODO-101 check for failure in write
  Wire.beginTransmission(addr);
  for (uint8_t i = 0; i < bytes; i++) {
    Wire.write(buf[i]);
  }
  Wire.endTransmission();
}
uint32_t _read(uint8_t buf, uint8_t bytes)
  Wire.requestFrom(addr, bytes);
  for (uint8_t i = 0; i < bytes; i++) {
    buf[i] = Wire.read();
    // If busyWhen is set, and r matches the mask then read again
    // TODO-101 check if should only do this on first byte
    while (r & busyWhen) { // Wait for device to be ready
      delay(10);
      buf[i] = Wire.read();
    }
  }
  #ifdef SYSTEM_I2C_DEBUG
    Serial.print("I2C read");
    for (uint8_t i = 0; i < bytes; i++) {
      Serial.print(buf[i], HEX); Serial.print(" ");
    }
    Serial.println();
  #endif
  return buf;
}
uint32_t _read(uint8_t bytes)
  Wire.requestFrom(addr, bytes);
  uint32_t result = 0;
  for (uint8_t i = 0; i < bytes; i++) {
    result = result << 8;
    uint8_t r = Wire.read();
    // If busyWhen is set, and r matches the mask then read again
    // TODO-101 check if should only do this on first byte
    while (r & busyWhen) { // Wait for device to be ready
      delay(10);
      r = Wire.read();
    }
    result |= r; // read one byte
  }
  #ifdef SYSTEM_I2C_DEBUG
    Serial.print("I2C read");  Serial.println(result); 
  #endif
  return result;
}
uint32_t System_I2C::sendAndRead(uint8_t cmd, uint8_t readBytes) {
  send(cmd);
  return _read(readBytes);
}

uint32_t System_I2C::sendAndRead(uint8_t* buf, uint8_t sendBytes, uint8_t readBytes) {
  send(buf, sendBytes);
  return _read(readBytes);
}

// Short cuts to send single byte and read 1 or 2 bytes
uint16_t System_I2C::read16(uint8_t cmd) {
  return (uint16_t) read(cmd, 2);  
}
uint8_t System_I2C::read8(uint8_t cmd) {
  return (uint8_t) read(cmd, 1);  
}
// readAndSet should be defined in subclass and call these functions

#endif //SYSTEM_I2C_WANT
