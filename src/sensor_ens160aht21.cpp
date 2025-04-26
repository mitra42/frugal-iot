/* Frugal IoT - Sensor - ENS160 AHT21
* 
* This is a sensor that uses the ENS160 and AHT21 chips to read the air quality.
*
* Info: https://www.instructables.com/ENS160-AHT21-Sensor-for-Arduino/
* Source: https://www.aliexpress.us/w/wholesale-ens160%2Baht21.html?
* Issue: https://github.com/mitra42/frugal-iot/issues/101
* Reddit: https://www.reddit.com/r/arduino/comments/12ulwo2/has_anyone_been_able_to_get_ensaht_working/
* 
* Important notes extracte from above.
* Vin is 5V - dont use 3.3V its an output from the regulator
*
* Thanks for lessons learned and some ideas/bits copied from https://github.com/adafruit/Adafruit_AHTX0
*
* //TODO-101 also review https://registry.platformio.org/libraries/k0i05/esp_ahtxx see if missed anything
*/

#include "_settings.h"  // Settings for what to include etc
#ifdef SENSOR_ENSAHT_WANT
#include <Arduino.h>
#include "sensor_ensaht.h"
#include "system_i2c.h"
#endif

#ifndef SENSOR_ENSAHT_AHTI2C
  #define SENSOR_ENSAHT_AHTI2C 0x38 // AHT default I2C address (alternate is 0x38)
#endif
#define AHTX0_CMD_CALIBRATE 0xE1     ///< Calibration command
#define AHTX0_CMD_TRIGGER 0xAC       ///< Trigger reading command
#define AHTX0_CMD_SOFTRESET 0xBA     ///< Soft reset command
#define AHTX0_STATUS_BUSY 0x80       ///< Status bit for busy
#define AHTX0_STATUS_CALIBRATED 0x08 ///< Status bit for calibrated
#define AHTX0_STATUS_REGISTER 0x71 // Only in KO105 - not in Adafruit library

#define SENSOR_ENSAHT_STRATEGY1  // Read till dont read 0x80
// #define SENSOR_ENSAHT_STRATEGY2 // Read status register till dont read 0x80

Sensor_ensaht::Sensor_ensaht(const char* name, const uint8_t AHTaddr) 
  //: Sensor(name, 10000, false), 
  //interface(addr) // I2C object at this address
: Sensor(name, 10000, false), 
  interface1(SENSOR_ENSAHT_AHTI2C) // I2C object at this address
  interface2(SENSOR_ENSAHT_ENSI2C) // I2C object at this address
{
  // TODO-101 add in OUTfloat for the ENS160 and AHT21 
  xxx = new OUTfloat("pressure", 0, "pressure", 0, 99, "blue", false);
  yyy = new OUTfloat("pressure", 0, "pressure", 0, 99, "blue", false);
}

//Sensor_ensaht::~Sensor_ensaht; //TODO-101

#ifdef SENSOR_ENSAHT_STRATEGY2
  uint8_t spinTillReady() {
    uint8_t status;
    do {
      delay(10);
      status = aht.read8(AHTX0_STATUS_REGISTER);
    } while (status & AHTX0_STATUS_BUSY);
    #ifdef SENSOR_ENSAHT_DEBUG
      Serial.print("AHTx ready"); // TODO-101 delete this as soon as confirm SENSOR_ENSAHT_STRATEGY2 works
    #endif
    return status;
  }
#endif // SENSOR_ENSAHT_STRATEGY2

void Sensor_ensaht::setup() {
  #ifdef SENSOR_MS5803_DEBUG
    Serial.println("ENS160 AHT21 Setup");
  #endif
  xxx->setup(name);
  yyy->setup(name);
  aht.initialize(AHTaddr);  // calls Wire.begin()
  #ifdef SENSOR_ENSAHT_STRATEGY1 //TODO101 try both
    aht.busyWhen(AHTX0_STATUS_BUSY);
  #endif
  ens.initialize(ENSaddr);  // calls Wire.begin() (unnecessary sinxe already called)

  // Setup the AHT21
  aht.send8(AHTX0_CMD_SOFTRESET); // Just waiting for not busy
  #ifdef SENSOR_ENSAHT_STRATEGY2
    spinTillReady();
  #endif
  uint8_t* cmd[3] = { AHTX0_CMD_CALIBRATE, 0x08, 0x00 };
  // TODO-101 Note the Adafruit library spins till it gets a status not 0x80 then reads next byte - not sure why but maybe need to do the same.
  // TODO-101 and KO105 says to read the status register first (0x71) till not busy
  #ifdef SENSOR_ENSAHT_STRATEGY1
    uint8_t status = aht.sendAndRead(cmd, 3, 1); // See note on Adafruit about not working on newer AHT20s
  #else // SENSOR_ENSAHT_STRATEGY2
    aht.send(cmd, 3);
    spinTillReady();
    uint8_t status = aht.read8(AHTX0_STATUS_REGISTER);
  #endif
  #ifdef SENSOR_ENSAHT_DEBUG
    Serial.print(F("AHT status (wanting & 0x08): "));
    Serial.println(status, HEX); 
  #endif

  // Setup the ENS160
  // TODO-101 ENS160 setup
}
void Sensor_ensaht::readAndSet() {
  // Read the AHT21
  uint8_t* cmd[3] = { AHTX0_CMD_TRIGGER, 0x33, 0x00 };
  uint8_t* data[6];
  uint8_t status = aht.sendAndRead(cmd, 3, &data, 6); // See note on Adafruit about not working on newer AHT20s
  // From the Adafruit library
  // ((uint32_t)rx[1] << 12) | ((uint32_t)rx[2] << 4) | (rx[3] >> 4); // From KO105 agrees
  uint32_t h = data[1];
  h <<= 8;
  h |= data[2];
  h <<= 4;
  h |= data[3] >> 4;
  float _humidity = ((float)h * 100) / 0x100000;
  #ifdef SENSOR_ENSAHT_DEBUG
    Serial.print(F("AHT humidity: "));
    Serial.println(_humidity);
  #endif
  // ((uint32_t)(rx[3] & 0x0f) << 16) | ((uint32_t)rx[4] << 8) | rx[5]; agrees with KO105
  uint32_t t = data[3] & 0x0F;
  t <<= 8;
  t |= data[4];
  t <<= 8;
  t |= data[5];
  float _temp = ((float)t * 200) / 0x100000 - 50;
  #ifdef SENSOR_ENSAHT_DEBUG
    Serial.print(F("AHT temperature: "));
    Serial.println(_temp);
  #endif  

  // Read the ENS160
  // TODO-101 ENS160 read

}


#define SENSOR_MS5803_DEBUG

