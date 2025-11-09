/* Frugal IoT - Load Sensor 
 * 
 * This is a load sensor that uses the HX711 chip to read the load cell.
 * 
 * WORK IN PROGRESS - while tested it still needs work to be more useful.
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

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include <HX711.h> // https://registry.platformio.org/libraries/robtillaart/HX711
#include "sensor_loadcell.h"
#include "misc.h" // StringF
#include "Frugal-IoT.h" // For frugal_iot

//TODO-134 need to tell it the size of the load cell
Sensor_LoadCell::Sensor_LoadCell(const char* const id, const char * const name, float max, const char* color, const bool retain, 
  uint8_t DOUTpin, uint8_t SCKpin, uint8_t times, int32_t offset, int32_t scale)
  : Sensor_Float(id, name, 3, 0, max, color, retain), 
    hx711(new HX711()),
    // TODO-C37 normalize times/offset/scale for other analog
    times(times),
    offset(offset), 
    scale(scale)
  {
    hx711->begin(DOUTpin, SCKpin);
  }
// This may also get set by a button or a message
void Sensor_LoadCell::tare() {
  hx711->tare(10); // Presume empty load cell   reading 10 tines for accuracy
  offset = hx711->get_offset(); // Get and save the offset value 
  #ifdef SENSOR_LOADCELL_DEBUG
    Serial.print(F("Loadcell tare: offset=")); Serial.println(offset);
  #endif
}
void Sensor_LoadCell::setup() {
  Sensor_Float::setup(); // Will readConfigFromFS and set offset and scale (thru to hx711), do before setting up pins
  hx711->reset(); // Set to initial state -do this because we dont know it was power cycled when dev board power cycles
  if (!hx711->get_tare()) { // Check if an offset has been set
    if (offset) {
      #ifdef SENSOR_LOADCELL_DEBUG
        Serial.print(F("Sensor_LoadCell::setup() - tare not set - using")); Serial.println(offset);
      #endif    
      hx711->set_offset(offset); // TODO-134 check this is correct
    } 
  }
#ifdef SENSOR_LOADCELL_DEBUG
  Serial.print(F("LoadCell: Scale=")); Serial.println(scale);
#endif
  hx711->set_scale(scale); // TODO-134 
  hx711->set_median_mode(); // Use median so doesn't read e.g. 0.5kg if 1kg load added mid-cycle

#ifdef SENSOR_LOADCELL_DEBUG
  Serial.print(F("Loadcell: Raw read=")); Serial.println(hx711->read());
  Serial.println(F("Loadcell: Put some weight on the scale in next 5 secs")); delay(5000);
  Serial.print(F("Loadcell: Raw read with weight=")); Serial.println(hx711->read());
  Serial.println(F("Loadcell get_units=")); Serial.println(hx711->get_units(1));
#endif
}
float Sensor_LoadCell::readFloat() {
  // Retuns actual value - so validate and set are defaults
  float v = hx711->get_units(times); 
  #ifdef SENSOR_LOADCELL_DEBUG
    Serial.print(F("LoadCell: get_units=")); Serial.println(v);
  #endif
  return v;
}
void Sensor_LoadCell::calibrate(float weight) {
  hx711->calibrate_scale(weight, 15); // Use maximmum of 15 
  scale = hx711->get_scale(); // Remember the value
  #ifdef SENSOR_LOADCELL_DEBUG
    Serial.print(F("LoadCell: scale=")); Serial.println(scale);
  #endif
}

void Sensor_LoadCell::dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet) {
  if (topicSensorId == id) {
    // Set by UX - "Tare" is weight=0  Calbrate is weight=XX
    if (topicTwig == "output") {
      if(payload.toFloat() == 0.0) {
        tare(); // sets offset on hx711 and here
        writeConfigToFS("offset", String(offset));
      } else {
        calibrate(payload.toFloat());
        writeConfigToFS("scale", String(scale));
      }
    // offset and scale should only be seen when reading from disk
    } else if (topicTwig == "offset") {
      hx711->set_offset(payload.toInt());
    } else if (topicTwig == "scale") {
      hx711->set_scale(payload.toFloat());
    } else {
      Sensor_Float::dispatchTwig(topicSensorId, topicTwig, payload, isSet);
    }
  }
}
void Sensor_LoadCell::captiveLines(AsyncResponseStream* response) {
  //TODO add specific Tare button
  frugal_iot.captive->addNumber(response, id, "output", String(output->value,3), name, 0, output->max);
  // Could add Tare as button - but probably want immediate response, not waiting on SEND
}

