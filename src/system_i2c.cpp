/* Frugal IoT System I2C 
 * Support for generic I2C interface
 * Note that some senors use I2C via other included libraries
 * 
 * NOTE THIS IS UNTESTED - IT MAY NOT WORK !!!!!! 
 */
#include "_settings.h"
#ifdef SYSTEM_I2C_WANT
#include <Wire.h>
#include "system_i2c.h"

System_I2C::System_I2C(uint8_t addr) : addr(addr) {}

void System_I2C::initialize() {
  Wire.begin(); 
}

// The raw send and write. 
// Send a single byte
void System_I2C::send(uint8_t cmd) {
  // TODO-101 check for failure in write
  Wire.beginTransmission(addr);
  Wire.write(cmd);
  Wire.endTransmission();
}
// Send buffer to I2C - arbitrary length
void System_I2C::send(uint8_t &buf, bytes) {
  // TODO-101 check for failure in write
  Wire.beginTransmission(addr);
  for (uint8_t i = 0; i < bytes; i++) {
    Wire.write(buf[i]);
  }
  Wire.endTransmission();
}
// Read buffer from I2C - arbitrary length
void read(uint8_t* buf, uint8_t bytes) {
  Wire.requestFrom(addr, bytes);
  for (uint8_t i = 0; i < bytes; i++) {
    buf[i] = Wire.read();
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
// Read from I2C - up to 4 bytes into a uint32_t
uint32_t read(uint8_t bytes) {
  Wire.requestFrom(addr, bytes);
  uint32_t result = 0;
  for (uint8_t i = 0; i < bytes; i++) {
    result = result << 8;
    result |= Wire.read();
  }
  #ifdef SYSTEM_I2C_DEBUG
    Serial.print("I2C read");  Serial.println(result); 
  #endif
  return result;
}

// Now various combinations used by different sensors - some will be in the sensor classes instead.
// TODO-101 rewrite ms803 to use this

uint8_t send1read1(uint8_t cmd) {
  send(cmd);
  return read(1);
}

#endif //SYSTEM_I2C_WANT
