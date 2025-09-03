/*
 * Temperature and Humidity sensor, 
 * Based on SHT85_demo_async.ino in https://github.com/RobTillaart/SHT85
 * 
 * Mitra Ardron: Sept 2024...Jun 2025
 * 
 * See the for guide to building one at 
 * https://github.com/mitra42/frugal-iot/wiki/Building-a-temperature---humidity-sensor/ 
 * 
 * Tested on:
 * Sensors: Lolin SHT30; a nice one on a cord, including some really cheap ones - no known compatability issues
 * Dev boards: ESP8266 & ESP32 on multiple boards - no known compatability issues
 * 
 * Required
 * Optional (default)
 * SENSOR_SHT_DEVICE (SHT30) // Which kind of device, for now it presumes they are all the same.
 * SENSOR_SHT_DEBUG          // Debugging output
 * 
 * Note - following is only used in main.cpp for constructor wth default in .h
 * SENSOR_SHT_ADDRESS (0x44) // Device address - 0x45 is also common esp D1 SHT shield
 *
 *
 * TODO-16 Support multiple I2C Wires - so for example can use two sensors on each wire. See Issue#16
 * TODO-16 Pull the Wire support into a seperate module so that a single Wire can be used for alternate sensors. See Issue#16
 * TODO Support I2C multiplexors - see sample code at https://github.com/RobTillaart/SHT85/issues/26#issuecomment-2367448245
*/

#ifndef SENSOR_SHT_H
#define SENSOR_SHT_H



#include <SHT85.h>
#include "sensor_ht.h"

#ifndef SENSOR_SHT_DEVICE
  #define SENSOR_SHT_DEVICE SHT30 // e.g. The Lolin SHT30 shield
#endif
// Has to be in .h, so can be used in main.cpp constructor
#ifndef SENSOR_SHT_ADDRESS
  // TODO build address this into OTA Key as requires two binaries
  #define SENSOR_SHT_ADDRESS 0x44 // Either 0x44 (small cheap ones we use or Deeley) 0x45 (D1 shield)
#endif

class Sensor_SHT : public Sensor_HT {
  public:
    Sensor_SHT(const char * const name, uint8_t address, TwoWire *wire, bool retain);
  protected:
    uint8_t address;
    SENSOR_SHT_DEVICE *sht; 
    void readValidateConvertSet() override; // Combines function of set(read()) since read gets two values from sensor
};

extern Sensor_SHT sensor_sht;

#endif // SENSOR_SHT_H
