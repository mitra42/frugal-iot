#ifndef SENSOR_ENSAHT_H
#define SENSOR_ENSAHT_H
#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "system_i2c.h"
#include "sensor.h"

class Sensor_ensaht : public Sensor {
  public:
    Sensor_ensaht(const char* const id, const char* const name, TwoWire* wire = &I2C_WIRE);
    ~Sensor_ensaht(); //TODO-101
  protected:
    OUTfloat* humidity;  
    OUTfloat* temperature;
    OUTuint16* aqi;
    OUTuint16* tvoc;
    OUTuint16* eco2;
    OUTuint16* aqi500; // Only valid on ENS161
    System_I2C* aht; // I2C object for AHT
    System_I2C* ens; // I2C object for ENS
    bool isENS161; // AQI is only on ENS160 not ENS160
    void setup() override; 
    void readValidateConvertSet() override;
    void readAndSetAHT();
  private:
    TwoWire* wire;
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
 };
 
#endif // SENSOR_ENSAHT_H
