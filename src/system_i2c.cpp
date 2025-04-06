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

void System_I2C::initialize() {
  Wire.begin(); 
}
void System_I2C::send(uint8_t cmd) {
  Wire.beginTransmission(addr);
  Wire.write(cmd);
  Wire.endTransmission();
}
uint32_t System_I2C::read(uint8_t cmd, uint8_t bytes) {
  send(cmd);
  Wire.requestFrom(addr, bytes);
  uint32_t result = 0;
  for (uint8_t i = 0; i < bytes; i++) {
    result = result << 8;
    result |= Wire.read(); // read one byte
  }
  Serial.print("XXX read");  Serial.println(result); 
  return result;
}
uint16_t System_I2C::read16(uint8_t cmd) {
  return (uint16_t) read(cmd, 2);  
}
// readAndSet should be defined in subclass and call these functions

#endif //SYSTEM_I2C_WANT
