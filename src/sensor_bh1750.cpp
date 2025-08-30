/* 
 * BH1750 light sensor as in Lilygo HiGrow
 * 
 * Optional: SENSOR_BH1750_ADDRESS
 */

#include "_settings.h" // Defines I2C_WIRE as either Wire or Wire1
#include "sensor_bh1750.h"
#include <BH1750.h>             //https://github.com/claws/BH1750

// TODO need a way to useful handle logarithnic values like lux - more of a UX issue than a node issue
// Practical range of lux unknown - apparantly can go from 0.001 to 65k 
Sensor_BH1750::Sensor_BH1750( const char* const id, const char * const name, const uint8_t addr, TwoWire* wire, const bool retain)
  : Sensor_Float(id, name, 3, 0, 65000, "yellow", retain), 
    addr(addr), wire(wire), lightmeter(addr) {
  }

// TODO add to docs - BH1750 default I2C address is 0x23
void Sensor_BH1750::setup() {
  Sensor_Float::setup(); // Will readConfigFromFS - do before setting up pins
  //bool BH1750::begin(Mode mode, byte addr, TwoWire* i2c
  wire->begin(I2C_SDA, I2C_SCL); // Note potential conflict with I2C on SHT. TODO-115
  //(Mode mode = CONTINUOUS_HIGH_RES_MODE, byte addr = 0x23, TwoWire* i2c = nullptr)
  // TODO copy this pattern of public enum to other enums 
  if (!lightmeter.begin(BH1750::Mode::CONTINUOUS_HIGH_RES_MODE, addr, wire)) { // (Mode mode, byte addr, TwoWire* i2c)
    Serial.println(F("Warning: Failed to initialize BH1750 light sensor!")); delay(5000);
  }
}
float Sensor_BH1750::read() {
  float v;
  v = lightmeter.readLightLevel();
  if (isnan(v)) {
      v = 0.0;
  }
  #ifdef SENSOR_BH1750_DEBUG
    Serial.print(F("BH1750:")); Serial.println(v);
  #endif
  return v;
}
