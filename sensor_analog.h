#ifndef SENSOR_ANALOG_H
#define SENSOR_ANALOG_H

/* Configuration options
 * Required: SENSOR_ANALOG_PIN
 * Optional: SENSOR_ANALOG_MS SENSOR_ANALOG_REFERENCE SENSOR_ANALOG_SMOOTH
*/


namespace sAnalog {
//  https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
// TODO what are the values on ESP8266 or ESP32
// TODO map between one set of REFERENCE values and the board specfic ones from the docs 
// See https://github.com/mitra42/frugal-iot/issues/60
#ifndef SENSOR_ANALOG_REFERENCE
  #ifdef ESP8266_D1_MINI
    #define SENSOR_ANALOG_REFERENCE DEFAULT
  #else
    #error analogReference() is board dependent, review the docs and online and define 
  #endif
#endif //  SENSOR_ANALOG_REFERENCE
#ifdef SENSOR_ANALOG_SMOOTH
extern unsigned long smoothedValue;
#endif
extern int value;
void setup();
void loop();
} // namespace sAnalog
#endif // SENSOR_ANALOG_H