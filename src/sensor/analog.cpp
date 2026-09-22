/*
 * Sensor Analog
 * Read from a pin and send message
 *
 * See https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html for lots more on ESP ADCs
 *
 * Configuration options.
 * Optional: SENSOR_ANALOG_REFERENCE for ESP8266 only  // TODO-141 phase out
 * Optional: SENSOR_ANALOG_ATTENTUATION // TODO-141 phase out
 * Optional: SENSOR_ANALOG_RESOLUTION - bits analogRead() returns; defaults to 12 on ESP32,
 *           which is what every raw-count constant in this library assumes. See the block
 *           below - the ESP32-S2 and S3 default to 13 and silently halve every such constant.
 * TODO: There is a lot more clever stuff on https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html
 * Its ESP32 specific, but looks like a range of capabilities that could be integrated.
 * 
 * On C3 - pin 0,1,4 works  5 gets error message  3 is Vbatt. 2 just reads 4095; 8,10 just reads 0; 7 reads 0 ad seems connected to LED
 */

#include "_settings.h"  // Settings for what to include etc
#include "sensor/analog.h"
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

/* SENSOR_ANALOG_RESOLUTION - how many bits analogRead() returns.
 *
 * Every raw-count constant in this library assumes 12 bits, i.e. 0..4095 - Sensor_Tank's
 * SENSOR_TANK_RAW_FULL and SENSOR_TANK_RAW_DISCONNECTED, and anything a user tares or
 * calibrates by hand against a number they read off the portal once.
 *
 * The ESP32 Arduino core does NOT default to 12 everywhere. It sets __analogReturnedWidth to
 * SOC_ADC_RTC_MAX_BITWIDTH, which is 13 on the S2 and S3 and 12 on the original ESP32 and the
 * C3. So the same sketch, the same divider and the same sensor give readings a factor of two
 * apart between boards, and every constant above is silently wrong on half of them. On an S2 a
 * full tank reads ~1407 against a 703 "full" constant, converts to ~200% and is clamped to 100%
 * - so the tank appears full from about half way up, and nothing looks broken.
 *
 * Defaulting to 12 on all ESP32 variants makes those constants mean what they say. Override it
 * only if you want the extra bit AND have recalibrated every raw constant for your board.
 *
 * This does NOT affect Sensor_Voltage, and so not Sensor_Battery, which overrides readInt() with
 * analogReadMilliVolts() - that returns millivolts whatever the resolution is set to.
 */
#ifndef SENSOR_ANALOG_RESOLUTION
  #ifdef ESP32
    #define SENSOR_ANALOG_RESOLUTION 12
  #endif
#endif // SENSOR_ANALOG_RESOLUTION

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
  pinMode(pin, INPUT); // I don't think this is needed ? - note Sensor_Battery also does this because reads before setup
  #ifdef SENSOR_ANALOG_REFERENCE
    analogReference(SENSOR_ANALOG_REFERENCE); // TODO see TODO's in the sensor/analog.h
  #endif 
  /* Global, not per-pin - analogReadResolution() and analogSetAttenuation() both apply to the
   * whole ADC. Setting them once per sensor is therefore redundant, but harmless, and it keeps
   * the configuration next to the pinMode it belongs with rather than in a board init nobody
   * reads. See the note on SENSOR_ANALOG_RESOLUTION at the top of this file for why 12 matters.
   */
  #ifdef SENSOR_ANALOG_RESOLUTION
    analogReadResolution(SENSOR_ANALOG_RESOLUTION);
  #endif
  #ifdef SENSOR_ANALOG_ATTENTUATION
    // Was analogSetAttentuation(...) with no semicolon - misspelt and unterminated, so defining
    // SENSOR_ANALOG_ATTENTUATION did not change the attenuation, it failed to compile. Never
    // exercised, because nothing in the tree defines it.
    analogSetAttenuation((adc_attenuation_t)SENSOR_ANALOG_ATTENTUATION);
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
float Sensor_Analog::readValidateConvert() {
  // Note almost identical code in Sensor_Uint16 Sensor_Float & Sensor_Analog
  const int v = readInt();           // Read raw value from sensor
  #ifdef SENSOR_ANALOG_DEBUG
    Serial.print(id); Serial.print(F(" raw:")); Serial.print(v);
  #endif
  if (validate(v)) {        // Check if its valid
    const float vv = convert(v);  // Convert - e.g. scale and offset
    #ifdef SENSOR_ANALOG_DEBUG
      Serial.print(F(" converted ")); Serial.println(vv);
    #endif
    return vv;
  }
  #ifdef SENSOR_ANALOG_DEBUG
    Serial.println();
  #endif
  return NAN;
}
void Sensor_Analog::tare() {
  // Read and use the reading as the offset for a 0 value
  offset = readInt();
}
void Sensor_Analog::calibrate(const float val) {
  // Note this could calibrate off an invalid raw value, may want to check if see this behavior
  int v = readInt();
  #ifdef SENSOR_ANALOG_DEBUG
    Serial.println(F("Calibrating val=")); Serial.print(val); Serial.print(F(" read=")); Serial.print(v); Serial.print(F(" offset=")); Serial.print(offset); Serial.println("scale = val/(reading-offset)");
  #endif
  scale = val / (v - offset);
}
void Sensor_Analog::dispatch(System_Message &msg) {
  if (msg.module() == id) {
    // Set by UX - "Tare" is weight=0  Calibrate is weight=XX
    if (msg.leaf() == "output") {
      if (msg.payload.toFloat() == 0.0) {
        tare(); // sets offset
        #ifdef SENSOR_ANALOG_DEBUG
          Serial.print(F("Tare offset=")); Serial.println(offset);
        #endif
        writeConfigToFS("offset", String(offset));
      } else {
        calibrate(msg.payload.toFloat()); // uses offset, sets scale
        #ifdef SENSOR_ANALOG_DEBUG
          Serial.print(F("Calibrate scale=")); Serial.println(scale);
        #endif
        writeConfigToFS("scale", String(scale));
      }
    // offset and scale should only be seen when reading from disk
    } else if (msg.leaf() == "offset") {
      offset = msg.payload.toInt();
    } else if (msg.leaf() == "scale") {
      scale = msg.payload.toFloat();
    } else {
      Sensor::dispatch(msg);
    }
  }
}


#endif // SENSOR_ANALOG_UNSUPPORTED

