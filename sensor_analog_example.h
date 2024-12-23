#ifndef SENSOR_ANALOG_EXAMPLE_H
#define SENSOR_ANALOG_EXAMPLE_H

/* Configuration options
 * Required: SENSOR_ANALOG_PIN
 * Optional: SENSOR_ANALOG_MS SENSOR_ANALOG_REFERENCE SENSOR_ANALOG_SMOOTH
*/

#include "sensor_analog.h"

#ifndef SENSOR_ANALOG_EXAMPLE_TOPIC
  #define SENSOR_ANALOG_EXAMPLE_TOPIC "analog"
#endif
#ifndef SENSOR_ANALOG_EXAMPLE_NAME
  #define SENSOR_ANALOG_EXAMPLE_NAME "Analog"
#endif
#define SENSOR_ANALOG_EXAMPLE_ADVERTISEMENT "\n  -\n    topic: " SENSOR_ANALOG_EXAMPLE_TOPIC "\n    name: " SENSOR_ANALOG_EXAMPLE_NAME "\n    type: int\n    display: bar\n    min: 0\n    max: 6000\n    color: purple\n    rw: r"


//  https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
// TODO what are the values on ESP8266 or ESP32
// TODO map between one set of REFERENCE values and the board specfic ones from the docs 
// See https://github.com/mitra42/frugal-iot/issues/60
#ifndef SENSOR_ANALOG_REFERENCE
  #ifdef ESP8266_D1_MINI
    #define SENSOR_ANALOG_REFERENCE DEFAULT // TODO not clear if / where this is used 
  #else
    #ifndef LOLIN_C3_PICO
      #error analogReference() is board dependent, review the docs and online and define 
    #endif
  #endif
#endif //  SENSOR_ANALOG_REFERENCE

namespace sAnalogExample {

void setup();
void loop();
} // sAnalogExample

#endif // SENSOR_ANALOG_EXAMPLE_H
