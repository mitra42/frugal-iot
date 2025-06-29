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

//TODO-134 need to tell it the size of the load cell
Sensor_LoadCell::Sensor_LoadCell(const char* const id, const char * const name, float max, const char* color, const bool retain, 
  uint8_t DOUTpin, uint8_t SCKpin, uint8_t times, int32_t offset, int32_t scale)
  : Sensor_Float(id, name, 3, 0, max, color, retain), 
    offset(offset), 
    scale(scale),
    times(times) {
  hx711 = new HX711();
  hx711->begin(DOUTpin, SCKpin);
}
// This may also get set by a button or a message
void Sensor_LoadCell::tare() {
  Serial.print(__FILE__); Serial.println(__LINE__);
    hx711->tare(10); // Presume empty load cell   reading 10 tines for accuracy
    Serial.print(__FILE__); Serial.println(__LINE__);
  offset = hx711->get_offset(); // Get and save the offset value 
    Serial.print(__FILE__); Serial.println(__LINE__);
  }
void Sensor_LoadCell::setup() {
  #ifdef SENSOR_LOADCELL_DEBUG
    Serial.println(F("Sensor_LoadCell::setup()"));
  #endif
  hx711->reset(); // Set to initial state -do this because we dont know it was power cycled when dev board power cycles
  if (!hx711->get_tare()) { // Check if an offset has been set
    if (offset) {
      #ifdef SENSOR_LOADCELL_DEBUG
        Serial.print(F("Sensor_LoadCell::setup() - tare not set - using")); Serial.println(offset);
      #endif    
      hx711->set_offset(offset); // TODO-134 check this is correct
    } else {
      #ifdef SENSOR_LOADCELL_DEBUG
        Serial.println(F("LoadCell: Checking tare - presumes nothing loaded"));
      #endif
      tare();
      #ifdef SENSOR_LOADCELL_DEBUG
        Serial.print(F("LoadCell: Offset=")); Serial.println(hx711->get_offset());
      #endif
    }
  }
#ifdef SENSOR_LOADCELL_DEBUG
  Serial.print(F("LoadCell: Scale=")); Serial.println(scale);
#endif
hx711->set_scale(scale); // TODO-134 
  hx711->set_median_mode(); // Use median so doesn't read e.g. 0.5kg if 1kg load added mid-cycle
}
float Sensor_LoadCell::read() {
  float v = hx711->get_units(times); 
  #ifdef SENSOR_LOADCELL_DEBUG
    Serial.print(F("LoadCell: read()=")); Serial.println(v);
  #endif
  return v;
}
// TODO-134 this looks wrong - I think weight should be float
void Sensor_LoadCell::calibrate(float weight) {
  hx711->calibrate_scale(weight, 15); // Use maximmum of 15 
  scale = hx711->get_scale(); // Remember the value
}
