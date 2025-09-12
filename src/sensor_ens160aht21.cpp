/* Frugal IoT - Sensor - ENS160 AHT21
* 
* This is a sensor that uses the ENS160 and AHT21 chips to read the air quality.
*
* Some info I found for this online
* Info: https://www.instructables.com/ENS160-AHT21-Sensor-for-Arduino/
* Source: https://www.aliexpress.us/w/wholesale-ens160%2Baht21.html?
* Issue: https://github.com/mitra42/frugal-iot/issues/101
* Reddit: https://www.reddit.com/r/arduino/comments/12ulwo2/has_anyone_been_able_to_get_ensaht_working/
* 
* One of the articles says Vin is 5V - dont use 3.3V its an output from the regulator, but I had it work on 3V fine.
*
* For AHT21
* Thanks for lessons learned and some ideas/bits copied from https://github.com/adafruit/Adafruit_AHTX0
*
* For ENS160 
* Based on https://github.com/adafruit/ENS160_driver
* Note Adafruit's driver implemets additional functionality not used here, especially custom heaters
* Also extra commands in https://github.com/adafruit/ENS160_driver/blob/master/src/ScioSense_ENS160.h
* 
* Configuration
* SENSOR_ENSAHT_AHTI2C(0x38); and SENSOR_ENSAHT_ENSI2C(0x53) if using multiple sensors
* SENSOR_ENSAHT_DEBUG
*
* //TODO-101 and TODO-23 note ENS160_OPMODE_DEP_SLEEP
* //TODO-101 also review https://registry.platformio.org/libraries/k0i05/esp_ahtxx see if missed anything
*/

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "sensor_ens160aht21.h"
#include "system_i2c.h"

// The sensors can be configured at alternative addresses, via defines - these are the default on the combination board
#ifndef SENSOR_ENSAHT_AHTI2C
  #define SENSOR_ENSAHT_AHTI2C (0x38) // AHT default I2C address (alternate are 0x38 0x39)
#endif
#ifndef SENSOR_ENSAHT_ENSI2C
  #define SENSOR_ENSAHT_ENSI2C (0x53) // AHT default I2C address (alternates are 0x52 0x53)
#endif
// Subset of AHTX0 commands
#define AHTX0_CMD_CALIBRATE 0xE1     ///< Calibration command
#define AHTX0_CMD_TRIGGER 0xAC       ///< Trigger reading command
#define AHTX0_CMD_SOFTRESET 0xBA     ///< Soft reset command
#define AHTX0_STATUS_BUSY 0x80       ///< Status bit for busy
#define AHTX0_STATUS_CALIBRATED 0x08 ///< Status bit for calibrated
#define AHTX0_STATUS_REGISTER 0x71   // Only in KO105 - not in Adafruit library
// Subset of ENS commands from https://github.com/adafruit/ENS160_driver/blob/master/src/ScioSense_ENS160.h
#define ENS160_BOOTING             10     // ms Unsure why called ENS_BOOTING (in Adafruit) but its a delay after each send
#define ENS160_REG_PART_ID          	0x00 		// 2 byte register
#define ENS160_REG_OPMODE		    0x10
#define ENS160_REG_COMMAND		  0x12
#define ENS160_REG_TEMP_IN		  0x13
#define ENS160_REG_DATA_STATUS  0x20
#define ENS160_REG_DATA_AQI		  0x21
#define ENS160_REG_DATA_TVOC	  0x22
#define ENS160_REG_DATA_ECO2	  0x24			
#define ENS160_COMMAND_NOP      0x00
#define ENS160_COMMAND_CLRGPR   0xCC
#define ENS160_DATA_STATUS_NEWDAT 0x02
//#define ENS160_OPMODE_DEP_SLEEP 0x00 // Not used yet
#define ENS160_OPMODE_IDLE      0x01
#define ENS160_OPMODE_STD		    0x02
#define ENS160_OPMODE_RESET	    0xF0
#define ENS160_PARTID				    0x0160
#define ENS161_PARTID				    0x0161


Sensor_ensaht::Sensor_ensaht(const char* const id, const char* const name, TwoWire* wire) 
  : Sensor(id, name, false)
{ // TODO-101 try movign some of these into part above
  aht = new System_I2C(SENSOR_ENSAHT_AHTI2C, &I2C_WIRE);
  // AHT21
  temperature = new OUTfloat(id, "temperature", "Temperature", 0, 0, 0, 455, "red", false);
  humidity = new OUTfloat(id, "humidity", "Humidity", 0, 0, 0, 100, "blue", false);
  // ENS160
  ens = new System_I2C(SENSOR_ENSAHT_ENSI2C, &I2C_WIRE); // I2C object at this address
  aqi = new OUTuint16(id, "aqi", "AQI", 0, 0, 255, "purple", false); // TODO-101 set min/max
  tvoc = new OUTuint16(id, "tvoc", "TVOC", 0, 0, 99, "green", false); // TODO-101 set min/max
  eco2 = new OUTuint16(id, "co2", "eCO2", 0, 300, 900, "brown", false); // TODO-101 set min/max
  aqi500 = new OUTuint16(id, "aqi500", "AQI500", 0, 0, 99, "brown", false); // Only valid on ENS161  // TODO-101 set min/max
}

//Sensor_ensaht::~Sensor_ensaht; //TODO-101

uint8_t Sensor_ensaht::AHTspinTillReady() {
  uint8_t status;
  do {
    delay(10);
    status = aht->send1read1(AHTX0_STATUS_REGISTER);
  } while (status & AHTX0_STATUS_BUSY);
  #ifdef SENSOR_ENSAHT_DEBUG
    //Serial.println(F("AHTx ready;");
  #endif
  return status; // Will mostly be ignored
}

void Sensor_ensaht::setupAHT() {
  #ifdef SENSOR_ENSAHT_DEBUG
    Serial.println(F("AHT21 Setup;"));
  #endif
  humidity->setup();
  temperature->setup();
  aht->initialize();  // calls wire->begin()
  // Setup the AHT21
  aht->send(AHTX0_CMD_SOFTRESET); // Just waiting for not busy
  AHTspinTillReady();
  uint8_t cmd[3] = { AHTX0_CMD_CALIBRATE, 0x08, 0x00 };
  // TODO-101 Note the Adafruit library spins till it gets a status not 0x80 then reads next byte - not sure why but maybe need to do the same.
  // TODO-101 and KO105 says to read the status register first (0x71) till not busy
  if (!aht->send(cmd, 3)) {  // See note on Adafruit about Calibratye not working on newer AHT20s
    Serial.println(F("AHT calibrate failed")); // TODO not sure how to handle the error - maybe fail out completely.
  };
  #ifdef SENSOR_ENSAHT_DEBUG
    uint8_t status = 
  #endif
  AHTspinTillReady();
  
  #ifdef SENSOR_ENSAHT_DEBUG
    Serial.print(F("AHT status (wanting & 0x08): "));
    Serial.println(status, HEX); 
  #endif

}

// ENS uses a register value paradigm 
bool Sensor_ensaht::ENSsend2(uint8_t reg, uint8_t val) {
  // Note dont rely on return code - currently always true
  uint8_t cmd[2] = {reg, val};
  bool status = ens->send(cmd, 2); // TODO-101 check sense of return code from ens-send
  #ifdef SENSOR_ENSAHT_DEBUG
    if (!status) {
      Serial.print(F("ENS failed to send")); Serial.print(reg); Serial.println(val);
    }
  #endif
  delay(ENS160_BOOTING);
  return status; // Note have console logged status
}
bool Sensor_ensaht::ENSsetMode(uint8_t val) {
  return ENSsend2(ENS160_REG_OPMODE, val);
}
bool Sensor_ensaht::ENScommand(uint8_t val) {
  return ENSsend2(ENS160_REG_COMMAND, val);
}
bool Sensor_ensaht::ENSsendAndRead(uint8_t reg, uint8_t *buf, uint8_t num) {
  bool status = ens->sendAndRead(reg, buf, num);
  delay(ENS160_BOOTING);
  return status;
}

void Sensor_ensaht::setupENS() {
  uint8_t readbuffer[2];
  #ifdef SENSOR_ENSAHT_DEBUG
    Serial.println(F("ENS160 Setup"));
  #endif
  eco2->setup();
  aqi->setup();
  tvoc->setup();
  //ens.initialize();  // calls wire->begin() (unnecessary since already called)
  ENSsetMode(ENS160_OPMODE_RESET);
  // TODO-101 could check and report part id if want ???
  ENSsendAndRead(ENS160_REG_PART_ID, readbuffer, 2);
  #ifdef SENSOR_ENSAHT_DEBUG
    uint16_t part_id = readbuffer[0] | ((uint16_t)readbuffer[1] << 8);
    Serial.print(F("ENS160 partid="));
    switch (part_id) { 
      case ENS160_PARTID: 
        isENS161 = false;
        Serial.println(F("ENS160"));
        break;
      case ENS161_PARTID:
        isENS161 = true;
        Serial.print(F("ENS160"));
        break;
      default:
        Serial.println(F("Unknown"));
        break;
    }
  #endif
  ENSsetMode(ENS160_OPMODE_IDLE);
  ENScommand(ENS160_COMMAND_NOP);
  ENScommand(ENS160_COMMAND_CLRGPR);
  // uint8_t status = ens->send1read1(ENS160_REG_DATA_STATUS);  // We don't use this so skip unless need for debug
    // (ens160.setMode(ENS160_OPMODE_STD)
  ENSsetMode(ENS160_OPMODE_STD); // For TVOC and CO2 rather than custom reads
}

void Sensor_ensaht::setup() {
  Sensor::setup(); // Will readConfigFromFS - do before setting up pins
  setupAHT();
  setupENS();
}

void Sensor_ensaht::readAndSetAHT() {
  // Read the AHT21
  uint8_t cmd[3] = { AHTX0_CMD_TRIGGER, 0x33, 0x00 }; //TODO-101 0x33 should be a defined constant
  uint8_t data[6];
  uint8_t status;
  // Strange - appear to be doing this twice 
  aht->send(cmd, 3);
  AHTspinTillReady();
  status = aht->read(data, 6);
  if (!status) {
    Serial.print(F("AHT fail to read")); // Do this even if not debugging
  }
  // From the Adafruit library
  // ((uint32_t)rx[1] << 12) | ((uint32_t)rx[2] << 4) | (rx[3] >> 4); // From KO105 agrees
  uint32_t h = data[1];
  h <<= 8;
  h |= data[2];
  h <<= 4;
  h |= data[3] >> 4;
  humidity->set(((float)h * 100) / 0x100000);
  // ((uint32_t)(rx[3] & 0x0f) << 16) | ((uint32_t)rx[4] << 8) | rx[5]; agrees with KO105
  uint32_t t = data[3] & 0x0F;
  t <<= 8;
  t |= data[4];
  t <<= 8;
  t |= data[5];
  temperature->set( ((float)t * 200) / 0x100000 - 50);
}
bool Sensor_ensaht::setenvdata(float temp, float hum) {
	uint16_t t = (uint16_t)((temp + 273.15f) * 64.0f);
  uint16_t h = (uint16_t)(hum * 512.0f);
	uint8_t trh_in[5];	
	//temp = (uint16_t)((t + 273.15f) * 64.0f);
  trh_in[0]= ENS160_REG_TEMP_IN;
	trh_in[1] = t & 0xff;
	trh_in[2] = (t >> 8) & 0xff;
		//temp = (uint16_t)(h * 512.0f);
	trh_in[3] = h & 0xff;
	trh_in[4] = (h >> 8) & 0xff;
	return ens->send(trh_in, 5);	
}

void Sensor_ensaht::readAndSetENS() {	
  uint8_t readbuffer[7];
	setenvdata(temperature->floatValue(), humidity->floatValue());
  uint8_t status;
  do {
    delay(1);
    status = ens->send1read1(ENS160_REG_DATA_STATUS);
    #ifdef SENSOR_ENSAHT_DEBUG
      //Serial.print(F("e"));
    #endif
  } while (!(ENS160_DATA_STATUS_NEWDAT & status));
  ENSsendAndRead(ENS160_REG_DATA_AQI, readbuffer, 7);
  aqi->set(readbuffer[0]);
  tvoc->set(readbuffer[1] | ((uint16_t)readbuffer[2] << 8));
  eco2->set(readbuffer[3] | ((uint16_t)readbuffer[4] << 8));
  if (isENS161) {
    aqi500->set(((uint16_t)readbuffer[5]) | ((uint16_t)readbuffer[6] << 8));
  }
}
void Sensor_ensaht::readValidateConvertSet() {
    readAndSetAHT(); // Note the temp and humiity from here are sent to the ENS
    readAndSetENS();   
}

void Sensor_ensaht::discover() {
    temperature->discover();
    humidity->discover();
    aqi->discover();
    tvoc->discover();
    eco2->discover();
    if (isENS161) { aqi500->discover(); }
}
void Sensor_ensaht::dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet) {
  if (topicSensorId == id) {
    if (
      temperature->dispatchLeaf(topicTwig, payload, isSet) ||
      humidity->dispatchLeaf(topicTwig, payload, isSet) ||
      aqi->dispatchLeaf(topicTwig, payload, isSet) ||
      tvoc->dispatchLeaf(topicTwig, payload, isSet) ||
      eco2->dispatchLeaf(topicTwig, payload, isSet) ||
      aqi500->dispatchLeaf(topicTwig, payload, isSet)
    ) { // True if changed
      // Nothing to do on sensor
    }
    System_Base::dispatchTwig(topicSensorId, topicTwig, payload, isSet);
  }
}
