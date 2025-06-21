/* Frugal IoT MS803 pressure sensor support
 * 
 * This is freestanding code since I was unable to find an existing library that matched requirements.
 * 
 * Some code is inspired from https://github.com/vic320/Arduino-MS5803-14BA which does SPI but 
 * does not appear to be supported any more, and is not available from the Platform IO platform.
 * 
 * https://www.te.com/commerce/DocumentDelivery/DDEController?Action=showdoc&DocId=Data+Sheet%7FMS5803-14BA%7FB3%7Fpdf%7FEnglish%7FENG_DS_MS5803-14BA_B3.pdf%7FCAT-BLPS0013 is useful
 * For SPI: Wire PS low; SDI-MISO; SDO-MOSI; SCK-SCLK; AD/CS-SS
 * For I2C: Wire PS high; SDA-SDA; SCL-SCL; CSB=high for Address 0x76; CSB=low for Address 0x77
 * Note that PS and CSB are external pins or jumpers on the dev board 
 */


// ====== NOTE THIS IS NOT YET WORKING - NEEDS TESTING ======

#include "_settings.h"  // Settings for what to include etc
#ifdef SENSOR_MS5803_WANT
#include <Arduino.h>
#ifdef SENSOR_MS5803_I2C
  #include <Wire.h>
#endif

#include "sensor_ms5803.h"
#ifdef SENSOR_MS5803_SPI
#include "system_spi.h"
#elif defined(SENSOR_MS5803_I2C)
#include "system_i2c.h"
#endif


#define SENSOR_MS5803_DEBUG

// Sensor constants:
#define SENSOR_CMD_RESET      0x1E
#define SENSOR_CMD_ADC_READ   0x00
#define SENSOR_CMD_ADC_CONV   0x40
#define SENSOR_CMD_ADC_D1     0x00
#define SENSOR_CMD_ADC_D2     0x10
#define SENSOR_CMD_ADC_256    0x00
#define SENSOR_CMD_ADC_512    0x02
#define SENSOR_CMD_ADC_1024   0x04
#define SENSOR_CMD_ADC_2048   0x06
#define SENSOR_CMD_ADC_4096   0x08
#define SENSOR_CMD_COEFFICIENT0 0xA0
 
  // TODO-132 add to discovery - hook into auto for OUT
  // TODO-132 add to mqtt
  // TODO-132 need to use a slower clock when at distance
// Instantiate with  sensors.push_back(new Sensor_ms5803())
Sensor_ms5803::Sensor_ms5803(const char* const id, const char * const name) : 
  Sensor(id, name, false),
  #ifdef SENSOR_MS5803_SPI
    interface(SENSOR_MS5803_SPI, SPI_CLOCK_DIV64) // uses default pins
  #elif defined(SENSOR_MS5803_I2C)
    interface(SENSOR_MS5803_I2C)
  #endif
{
 
  pressure = new OUTfloat(id, "pressure", "Pressure", 0, 1, 0, 99, "blue", false);
  temperature = new OUTfloat(id, "temperature", "Temperature", 0, 1, 0, 99, "red", false);
}

void Sensor_ms5803::setup() {
  #ifdef SENSOR_MS5803_DEBUG
    Serial.println("MS5803 Setup");
  #endif
  pressure->setup(name);
  delay(100); // TODO XXX unsure if needed
  interface.initialize();
  delay(100); // TODO XXX unsure if needed
  interface.send(SENSOR_CMD_RESET);
  // These sensors have coefficient values stored in ROM that are used to convert the raw temp/pressure data into degrees and mbars.
	// Read sensor coefficients - these will be used to convert sensor data into pressure and temp data
  delay(100); // TODO XXX unsure if needed
  for (int i = 0; i < 8; i++ ){
    interface.send(SENSOR_CMD_COEFFICIENT0 + ( i * 2 ));  // read coefficients    
    sensorCoefficients[ i ] = (uint16_t)interface.read(2);  // read coefficients
    #ifdef SENSOR_MS5803_DEBUG
      Serial.print("Coefficient = ");
      Serial.println(sensorCoefficients[ i ]);
    #endif 
    delay(10);
  }
  // If the calculated CRC does not match the returned CRC, then there is a data integrity issue.
  // Check the connections for bad solder joints or "flakey" cables. 
  // If this issue persists, you may have a bad sensor.
  if (!ms5803CRC4()) {
    Serial.println(F("MS5803 bad CRC on coefficients"));
    // return false;
  } else {
    // If the CRC matches, then the sensor is good to go.
    Serial.println(F("MS5803 looks good"));
    // return true;
  }
}

// Coefficient at index 7 is a four bit CRC value for verifying the validity of the other coefficients.
// The value returned by this method should match the coefficient at index 7.
// If not there is something works with the sensor or the connection.
uint8_t Sensor_ms5803::ms5803CRC4() {
  uint8_t cnt;
  uint16_t n_rem;
  uint16_t crc_read; // TODO are these uint16_t or what ?
  uint8_t  n_bit;
  
  n_rem = 0x00;
  crc_read = sensorCoefficients[7];
  sensorCoefficients[7] = ( 0xFF00 & ( sensorCoefficients[7] ) );
  
  for (cnt = 0; cnt < 16; cnt++)
  { // choose LSB or MSB
      if ( cnt%2 == 1 ) n_rem ^= (uint16_t) ( ( sensorCoefficients[cnt>>1] ) & 0x00FF );
      else n_rem ^= (uint16_t) ( sensorCoefficients[cnt>>1] >> 8 );
      for ( n_bit = 8; n_bit > 0; n_bit-- )
      {
          if ( n_rem & ( 0x8000 ) )
          {
              n_rem = ( n_rem << 1 ) ^ 0x3000;
          }
          else {
              n_rem = ( n_rem << 1 );
          }
      }
  }
  
  n_rem = ( 0x000F & ( n_rem >> 12 ) );// // final 4-bit reminder is CRC code
  sensorCoefficients[7] = crc_read; // restore the crc_read to its original place
  
  return ( crc_read == (n_rem ^ 0x00 )); // The calculated CRC should match what the device initally returned.
}


void Sensor_ms5803::readAndSet() {
  interface.send(SENSOR_CMD_ADC_CONV | SENSOR_CMD_ADC_4096 | SENSOR_CMD_ADC_D2); 
  delay(100); // Wait for conversion to complete
  interface.send(SENSOR_CMD_ADC_READ); // read the ADC value
  uint32_t D2 = interface.read(3);  // uncompensated temperature
  // TODO-132 need to do the math to get the temperature
  interface.send(SENSOR_CMD_ADC_CONV | SENSOR_CMD_ADC_4096 | SENSOR_CMD_ADC_D1);
  delay(100); // Wait for conversion to complete //TODO note that vic320 had shorter delays
  interface.send(SENSOR_CMD_ADC_READ); // read the ADC value
  uint32_t D1 = interface.read(3); // uncompensated pressure
  // calculate 1st order pressure and temperature correction factors (MS5803 1st order algorithm). 
  float deltaTemp = D2 - sensorCoefficients[5] * pow( 2, 8 );
  float sensorOffset = sensorCoefficients[2] * pow( 2, 16 ) + ( deltaTemp * sensorCoefficients[4] ) / pow( 2, 7 );
  float sensitivity = sensorCoefficients[1] * pow( 2, 15 ) + ( deltaTemp * sensorCoefficients[3] ) / pow( 2, 8 );
  #ifdef SENSOR_MS5803_DEBUG
    Serial.print(F("D1 = "));
    Serial.println(D1);
    Serial.print(F("D2 = "));
    Serial.println(D2);
    Serial.print(F("deltaTemp = "));
    Serial.println(deltaTemp);
    Serial.print(F("sensorOffset = "));
    Serial.println(sensorOffset);
    Serial.print(F("sensitivity = "));
    Serial.println(sensitivity);
  #endif
    
  // calculate 2nd order pressure and temperature (MS5803 2st order algorithm)
  // will send to mqtt
  temperature->set(( 2000 + (deltaTemp * sensorCoefficients[6] ) / pow( 2, 23 ) ) / 100);  // in degrees C
  pressure->set(( ( ( ( D1 * sensitivity ) / pow( 2, 21 ) - sensorOffset) / pow( 2, 15 ) ) / 10 ));   // in mBars
}

void Sensor_ms5803::dispatchTwig(const String &topicSensorId, const String &leaf, const String &payload, bool isSet) {
  if (topicSensorId == id) {
    if (
      pressure->dispatchLeaf(leaf, payload, isSet) ||
      temperature->dispatchLeaf(leaf, payload, isSet)
    ) { // True if changed 
      // Nothing to do on Sensor
    }
  }
}

#endif // SENSOR_MS5803_WANT
