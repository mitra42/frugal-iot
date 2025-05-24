#ifndef SENSOR_SHT_H
#define SENSOR_SHT_H


#include <SHT85.h>
#include "sensor_ht.h"

#ifndef SENSOR_SHT_DEVICE
  #define SENSOR_SHT_DEVICE SHT30 // e.g. The Lolin SHT30 shield
#endif
#ifndef SENSOR_SHT_ADDRESS
  #define SENSOR_SHT_ADDRESS 0x44 // 0x45 is the D1 shield 0x44 is also common TODO build this into OTA Key as requires two binaries
#endif
#ifndef SENSOR_SHT_MS
  #define SENSOR_SHT_MS 60000
#endif

class Sensor_SHT : public Sensor_HT {
public:
  uint8_t address;
  SENSOR_SHT_DEVICE *sht; 
  Sensor_SHT(const char * const name, uint8_t address, TwoWire *wire, const unsigned long ms, bool retain);
  virtual void readAndSet(); // Combines function of set(read()) since read gets two values from sensor
};

extern Sensor_SHT sensor_sht;

#endif // SENSOR_SHT_H
