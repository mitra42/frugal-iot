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
#include "sensor_ens160aht21.h"
#include "system_i2c.h"

#ifndef SENSOR_ENSAHT_AHTI2C
  #define SENSOR_ENSAHT_AHTI2C (0x38) // AHT default I2C address (alternate is 0x38)
#endif
#define AHTX0_CMD_CALIBRATE 0xE1     ///< Calibration command
#define AHTX0_CMD_TRIGGER 0xAC       ///< Trigger reading command
#define AHTX0_CMD_SOFTRESET 0xBA     ///< Soft reset command
#define AHTX0_STATUS_BUSY 0x80       ///< Status bit for busy
#define AHTX0_STATUS_CALIBRATED 0x08 ///< Status bit for calibrated
#define AHTX0_STATUS_REGISTER 0x71 // Only in KO105 - not in Adafruit library

#define SENSOR_ENSAHT_STRATEGY1  // Read till dont read 0x80
#define SENSOR_ENSAHT_DEBUG
// #define SENSOR_ENSAHT_STRATEGY2 // Read status register till dont read 0x80

Sensor_ensaht::Sensor_ensaht(const char* const id, const char* const name) 
  //: Sensor(name, 10000, false), 
  //interface(addr) // I2C object at this address
: Sensor(id, name, 10000, false)
{
  aht = new System_I2C(SENSOR_ENSAHT_AHTI2C);
  //ens = new System_I2C(SENSOR_ENSAHT_ENSI2C); // I2C object at this address
  // TODO-101 add in OUTfloat for the ENS160 and AHT21 
  temperature = new OUTfloat(id, "temperature", "Temperature", 0, 0, 0, 99, "blue", false);
  humidity = new OUTfloat(id, "humidity", "Humidity", 0, 0, 0, 99, "blue", false);
}

//Sensor_ensaht::~Sensor_ensaht; //TODO-101

uint8_t Sensor_ensaht::spinTillReady() {
  uint8_t status;
  do {
    delay(10);
    status = aht->send1read1(AHTX0_STATUS_REGISTER);
  } while (status & AHTX0_STATUS_BUSY);
  #ifdef SENSOR_ENSAHT_DEBUG
    Serial.print("AHTx ready"); // TODO-101 delete this as soon as confirm SENSOR_ENSAHT_STRATEGY2 works
  #endif
  return status; // Will mostly be ignored
}

void Sensor_ensaht::setup() {
  #ifdef SENSOR_MS5803_DEBUG
    Serial.println("ENS160 AHT21 Setup");
  #endif
  // TODO-101 expand this to all the OUTxxx
  humidity->setup(name);
  temperature->setup(name);
  //ens->setup(name);
  Serial.print("XXX " __FILE__); Serial.println(__LINE__); delay(100);

  aht->initialize();  // calls Wire.begin()
  //ens.initialize();  // calls Wire.begin() (unnecessary sinxe already called)

  // Setup the AHT21
  Serial.print("XXX " __FILE__); Serial.println(__LINE__); delay(100);
  aht->send(AHTX0_CMD_SOFTRESET); // Just waiting for not busy
  Serial.print("XXX " __FILE__); Serial.println(__LINE__); delay(100);
  spinTillReady();
  Serial.print("XXX " __FILE__); Serial.println(__LINE__); delay(100);

  uint8_t cmd[3] = { AHTX0_CMD_CALIBRATE, 0x08, 0x00 };
  // TODO-101 Note the Adafruit library spins till it gets a status not 0x80 then reads next byte - not sure why but maybe need to do the same.
  // TODO-101 and KO105 says to read the status register first (0x71) till not busy
  if (!aht->send(cmd, 3)) {  // See note on Adafruit about Calibratye not working on newer AHT20s
    Serial.println(F("AHT calibrate failed")); // TODO not sure how to handle the error - maybe fail out completely.
  };
  Serial.print("XXX " __FILE__); Serial.println(__LINE__); delay(100);
  uint8_t status = spinTillReady();
  #ifdef SENSOR_ENSAHT_DEBUG
    Serial.print(F("AHT status (wanting & 0x08): "));
    Serial.println(status, HEX); 
  #endif

  // Setup the ENS160
  // TODO-101 ENS160 setup
}
void Sensor_ensaht::readAndSet() {
  // Read the AHT21
  Serial.print("XXX " __FILE__); Serial.println(__LINE__); delay(100);
  uint8_t cmd[3] = { AHTX0_CMD_TRIGGER, 0x33, 0x00 };
  uint8_t data[6];
  uint8_t status;
  Serial.print("XXX " __FILE__); Serial.println(__LINE__); delay(100);
  aht->send(cmd, 3);
  Serial.print("XXX " __FILE__); Serial.println(__LINE__); delay(100);
  spinTillReady();
  Serial.print("XXX " __FILE__); Serial.println(__LINE__); delay(100);
  status = aht->read(data, 6);
  Serial.print("XXX " __FILE__); Serial.println(__LINE__); delay(100);
  if (!aht->sendAndRead(cmd, 3, data, 6)) {
    Serial.print(F("AHT fail to read"));
  }
  Serial.print("XXX " __FILE__); Serial.println(__LINE__); delay(100);
  // From the Adafruit library
  // ((uint32_t)rx[1] << 12) | ((uint32_t)rx[2] << 4) | (rx[3] >> 4); // From KO105 agrees
  Serial.print("XXX " __FILE__); Serial.println(__LINE__); delay(100);
  uint32_t h = data[1];
  h <<= 8;
  h |= data[2];
  h <<= 4;
  h |= data[3] >> 4;
  float _humidity = ((float)h * 100) / 0x100000;
  // TODO-101 do something with _humidity
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
  // TODO-101 do something with _temp
  #ifdef SENSOR_ENSAHT_DEBUG
    Serial.print(F("AHT temperature: "));
    Serial.println(_temp);
  #endif  

  // Read the ENS160
  // TODO-101 ENS160 read

}



#endif // SENSOR_ENSAHT_WANT