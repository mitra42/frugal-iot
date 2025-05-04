/* Frugal IoT - Load Sensor 
 * 
 * This is a load sensor that uses the HX711 chip to read the load cell.
 * 
 * The load cell is a xKg load cell with a ??? output. 
 * The HX711 chip amplifies the signal and converts it to a digital value
 * 
 * TODO_134 TODO_POWER see https://registry.platformio.org/libraries/robtillaart/HX711 notes on power management
 * TODO_134 note there are two different HX711 chips, A & N - see notes in HX711 library
 */

#include "_settings.h"  // Settings for what to include etc
#ifdef SENSOR_LOADCELL_WANT

#include <Arduino.h>
#include <HX711.h> // https://registry.platformio.org/libraries/robtillaart/HX711
#include "sensor_loadcell.h"
#include "misc.h" // StringF

#ifndef SENSOR_LOADCELL_DOUT_PIN
  #ifdef ESP8266_D1
    #define SENSOR_LOADCELL_DOUT_PIN D5 // TODO-134 check pin numbers
  #elif defined(ESP32)
    #define SENSOR_LOADCELL_DOUT_PIN 1 // TODO-134 check pin numbers
  #endif
#endif
#ifndef SENSOR_LOADCELL_SCK_PIN
  #ifdef ESP8266_D1
    #define SENSOR_LOADCELL_SCK_PIN D0
  #elif defined(ESP32)
    #define SENSOR_LOADCELL_SCK_PIN 0
  #endif
#endif
#ifndef SENSOR_LOADCELL_OFFSET
  #define SENSOR_LOADCELL_OFFSET 0 // check this
#endif
#ifndef SENSOR_LOADCELL_SCALE
  #define SENSOR_LOADCELL_SCALE 2000 // check this
#endif
#ifndef SENSOR_LOADCELL_TIMES
  #define SENSOR_LOADCELL_TIMES 10 // TODO_134 check how much different numbers of reading effect accuracy
#endif
#ifndef SENSOR_LOADCELL_MAX
  #define SENSOR_LOADCELL_MAX 2000 // TODO-134 should probably be a required paramter
#endif

//TODO-134 need to tell it the size of the load cell
Sensor_LoadCell::Sensor_LoadCell(const char* const id, const char* name, float max, const char* color, const unsigned long ms, const bool retain)
  : Sensor_Float(id, name, 3, 0, max, color, ms, retain), 
    offset(SENSOR_LOADCELL_OFFSET), 
    scale(SENSOR_LOADCELL_SCALE) {
  hx711 = new HX711();
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
  hx711->begin(SENSOR_LOADCELL_DOUT_PIN, SENSOR_LOADCELL_SCK_PIN); // TODO-134 check pin numbers
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
  float v = hx711->get_units(SENSOR_LOADCELL_TIMES); 
  #ifdef SENSOR_LOADCELL_DEBUG
    Serial.print(F("LoadCell: read()=")); Serial.println(v);
  #endif
  return v;
}
// TODO-134 this looks wrong - I think weight should be float
void Sensor_LoadCell::calibrate(uint16_t weight) {
  hx711->calibrate_scale(weight, 15); // Use maximmum of 15 
  scale = hx711->get_scale(); // Remember the value
}


#endif //SENSOR_LOADCELL_WANT