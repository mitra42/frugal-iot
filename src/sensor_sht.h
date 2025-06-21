#ifndef SENSOR_SHT_H
#define SENSOR_SHT_H


#include <SHT85.h>
#include "sensor_ht.h"

#ifndef SENSOR_SHT_DEVICE
  #define SENSOR_SHT_DEVICE SHT30 // e.g. The Lolin SHT30 shield
#endif
#ifndef SENSOR_SHT_ADDRESS
  // TODO build address this into OTA Key as requires two binaries
  #define SENSOR_SHT_ADDRESS 0x44 // Either 0x44 (small cheap ones we use or Deeley) 0x45 (D1 shield)
#endif

class Sensor_SHT : public Sensor_HT {
public:
  uint8_t address;
  SENSOR_SHT_DEVICE *sht; 
  Sensor_SHT(const char * const name, uint8_t address, TwoWire *wire, bool retain);
  virtual void readAndSet(); // Combines function of set(read()) since read gets two values from sensor
};

extern Sensor_SHT sensor_sht;

#endif // SENSOR_SHT_H
