/* Frugal IoT - Load Sensor 
 * 
 * This is a load sensor that uses the HX711 chip to read the load cell.
 * 
 * The load cell is a xKg load cell with a ??? output. 
 * The HX711 chip amplifies the signal and converts it to a digital value
 * It is not one of the standard buses (I2C or SPI) but a custom protocol.
 * 
 * Cableing - to go in Docs  DOUT and SCK pins supplied to constructor
 * 
 * Configuration: SENSOR_LOADCELL_DEBUG
 *  
 * Usage .... need to tare which means to set the offset such that whatever the reading is represents zero. 
 * Then call calibrate(weight) to set the scale so that the current load should be reported as weight. 
 * 
 * The value passed to calibrate does not not have to be the weight - it could for example be the value of that amount.
 * 
 * TODO_134 TODO_POWER see https://registry.platformio.org/libraries/robtillaart/HX711 notes on power management
 * TODO_134 note there are two different HX711 chips, A & N - see notes in HX711 library
 */

 #ifndef SENSOR_LOADCELL_H
#define SENSOR_LOADCELL_H

/* Frugal IoT - load cell sensor
*/
#include "_settings.h"  // Settings for what to include etc
#include "sensor_float.h"
#include <HX711.h> // https://registry.platformio.org/libraries/robtillaart/HX711

class Sensor_LoadCell : public Sensor_Float {
  public:
    Sensor_LoadCell(const char* const id, const char * const name, float max, const char* color, const bool retain, 
      uint8_t DOUTpin, uint8_t SCKpin, uint8_t times, int32_t offset, int32_t scale);
  protected:
    float readFloat() override;
    void tare();
    void setup() override;
    void calibrate(float weight);
    HX711 *hx711;
    uint8_t times; // How often to read the load cell for each reported reading
    int32_t offset;
    float scale;
    void dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet) override;
    void captiveLines(AsyncResponseStream* response);
};
#endif // SENSOR_LOADCELL_H