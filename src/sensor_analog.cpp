/*
 * Sensor Analog
 * Read from a pin and send message
 *
 * See https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html for lots more on ESP ADCs
 *
 * Configuration options.
 * Optional: SENSOR_ANALOG_REFERENCE for ESP8266 only  // TODO-141 phase out
 * Optional: SENSOR_ANALOG_ATTENTUATION // TODO-141 phase out
 * TODO: There is a lot more clever stuff on https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html
 * Its ESP32 specific, but looks like a range of capabilities that could be integrated.
 * 
 * On C3 - pin 0,1,4 works  5 gets error message  3 is Vbatt. 2 just reads 4095; 8,10 just reads 0; 7 reads 0 ad seems connected to LED
 */

#include "_settings.h"  // Settings for what to include etc
#include "sensor_analog.h"
#include "Frugal-IoT.h"


#include <Arduino.h>

//TO_ADD_BOARD
//  https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
// TODO what are the values on ESP8266 or ESP32
// TODO map between one set of REFERENCE values and the board specfic ones from the docs 
// See https://github.com/mitra42/frugal-iot/issues/60
// TODO Note this is not going to make it to the place its used in sensor_analog
#ifndef SENSOR_ANALOG_REFERENCE
  #ifdef ESP8266_D1
    #define SENSOR_ANALOG_REFERENCE DEFAULT // TODO not clear if / where this is used 
  #elif defined(ESP32) // It doesnt seem to be used on ESP32s 
  #else
    #define SENSOR_ANALOG_UNSUPPORTED
    //#error analogReference() is processor dependent, review the docs and online and define
  #endif
#endif //  SENSOR_ANALOG_REFERENCE

#ifndef SENSOR_ANALOG_UNSUPPORTED
// If Analog unsupported then a linker error will be generated if try and add one. 

Sensor_Analog::Sensor_Analog(const char* const id, const char * const name, const uint8_t p, const uint8_t width, const float min, const float max, int offset, float scale, const char* color, bool r) 
: Sensor_Float(id, name, width, min, max, color, r),
  pin(p),
  offset(offset),
  scale(scale)
{ };


// Sensor_Uint16_t::act is obsolete
// Sensor_Uint16_t::set is good - does optional smooth, compares and sends
// Sensor_Uint16_t::periodically is good - does periodic read and set


void Sensor_Analog::setup() {
  Sensor_Float::setup(); // Will readConfigFromFS - do before setting up pin
  // initialize the analog pin as an input.
  pinMode(pin, INPUT); // I don't think this is needed ?
  #ifdef SENSOR_ANALOG_REFERENCE
    analogReference(SENSOR_ANALOG_REFERENCE); // TODO see TODO's in the sensor_analog.h
  #endif 
  #ifdef SENSOR_ANALOG_ATTENTUATION
    analogSetAttentuation(SENSOR_ANALOG_ATTENTUATION)
  #endif
}

// Note this is virtual, and overridden in Sensor_Battery
// Note Analog's read int, but set float after scaling
int Sensor_Analog::readInt() {
  return analogRead(pin); // Returns an int - which should be int16_t
}
// Check if its valid, typically this will be overridden
bool Sensor_Analog::validate(int v) {
  return true;
}

float Sensor_Analog::convert(const int v) {
  return (v - offset) * scale;
}
void Sensor_Analog::readValidateConvertSet() {
  // Note almost identical code in Sensor_Uint16 Sensor_Float & Sensor_Analog
  const int v = readInt();           // Read raw value from sensor
  #ifdef SENSOR_ANALOG_DEBUG
    Serial.print(id); Serial.print(F(" raw:")); Serial.print(v);
  #endif
  if (validate(v)) {        // Check if its valid
    const float vv = convert(v);  // Convert - e.g. scale and offset
    set(vv);                  // set - and send message
    #ifdef SENSOR_ANALOG_DEBUG
      Serial.print(F(" converted ")); Serial.print(vv);
    #endif
  }
  #ifdef SENSOR_ANALOG_DEBUG
    Serial.println();
  #endif
}
void Sensor_Analog::tare() {
  // Read and use the reading as the offset for a 0 value
  offset = readInt();
}
void Sensor_Analog::calibrate(const float val) {
  // Note this could calibrate off an invalid raw value, may want to check if see this behavior
  scale = val / (readInt() - offset);
}
void Sensor_Analog::dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, const bool isSet) {
  if (topicSensorId == id) {
    // Set by UX - "Tare" is weight=0  Calbrate is weight=XX
    if (topicTwig == "output") {
      if(payload.toFloat() == 0.0) {
        tare(); // sets offset
        #ifdef SENSOR_ANALOG_DEBUG
          Serial.print(F("Tare offset=")); Serial.println(offset);
        #endif
        writeConfigToFS("offset", String(offset));
      } else {
        calibrate(payload.toFloat()); // uses offset, sets scale
        #ifdef SENSOR_ANALOG_DEBUG
          Serial.print(F("Calibrate scale=")); Serial.println(scale);
        #endif
        writeConfigToFS("scale", String(scale));
      }
    // offset and scale should only be seen when reading from disk
    } else if (topicTwig == "offset") {
      offset = payload.toInt();
    } else if (topicTwig == "scale") {
      scale = payload.toFloat();
    } else {
      Sensor_Float::dispatchTwig(topicSensorId, topicTwig, payload, isSet);
    }
  }
}


#endif // SENSOR_ANALOG_UNSUPPORTED

