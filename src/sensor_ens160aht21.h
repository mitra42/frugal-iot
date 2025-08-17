#ifndef SENSOR_ENSAHT_H
#define SENSOR_ENSAHT_H
#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "system_i2c.h"
#include "sensor.h"

class Sensor_ensaht : public Sensor {
  public:
    //TODO - create a base class Sensor_multiple that has an outputs list and dispatchTwig iterates over them - keep these pointers as well (notice how system_frugal_iot creates things and adds to an array)
    //TODO - then use for other multi-output sensors (like sensor_ht) and then maybe for single sensors, simplifying e,g. dispatchTwig in the process
    OUTfloat* humidity;  
    OUTfloat* temperature;
    OUTuint16* aqi;
    OUTuint16* tvoc;
    OUTuint16* eco2;
    OUTuint16* aqi500; // Only valid on ENS161
    System_I2C* aht; // I2C object for AHT
    System_I2C* ens; // I2C object for ENS
    bool isENS161;
    Sensor_ensaht(const char* const id, const char* const name);
    ~Sensor_ensaht(); //TODO-101
    void setup() override; 
    void readAndSet() override;
    void readAndSetAHT();
  private:
    // AHT
    void setupAHT();
    uint8_t AHTspinTillReady();
    // ENS
    void setupENS();
    void readAndSetENS();
    bool ENSsend2(uint8_t reg, uint8_t val);
    bool ENSsetMode(uint8_t val);
    bool ENScommand(uint8_t val);
    bool ENSsendAndRead(uint8_t reg, uint8_t *buf, uint8_t num);
    bool setenvdata(float temp, float hum);
  #ifdef SYSTEM_DISCOVERY_SHORT
    void discover() override;
  #else
    String advertisement() override;
  #endif
    void dispatchTwig(const String &topicSensorId, const String &leaf, const String &payload, bool isSet);
 };
 
#endif // SENSOR_ENSAHT_H
