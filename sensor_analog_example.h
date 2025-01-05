#ifndef SENSOR_ANALOG_EXAMPLE_H
#define SENSOR_ANALOG_EXAMPLE_H

/* Configuration options
 * Required: SENSOR_ANALOG_EXAMPLE_PIN
 * Optional: SENSOR_ANALOG_EXAMPLE_MS SENSOR_ANALOG_REFERENCE SENSOR_ANALOG_EXAMPLE_SMOOTH SENSOR_ANALOG_EXAMPLE_NAME
*/

#include "sensor_analog.h"

#ifndef SENSOR_ANALOG_EXAMPLE_TOPIC
  #define SENSOR_ANALOG_EXAMPLE_TOPIC "analog"
#endif
#ifndef SENSOR_ANALOG_EXAMPLE_NAME
  #define SENSOR_ANALOG_EXAMPLE_NAME "Analog"
#endif

#ifndef SENSOR_ANALOG_EXAMPLE_PIN
  #ifdef ESP8266_D1_MINI
    // Only one analog pin on D1 Mini
    #define SENSOR_ANALOG_EXAMPLE_PIN A0
  #else
    #ifdef LOLIN_C3_PICO
      // 0,1,4 work 5 gets error message; 3 is Vbatt; 2 just gets 4095, 8,10 get 0, 7 gets 0 and seems connected to LED
      #define SENSOR_ANALOG_EXAMPLE_PIN 4 // Which pin to read: 
    #else
      #error Sorry no default Analog pin for your board // #TO_ADD_BOARD
    #endif
  #endif
#endif

#ifndef SENSOR_ANALOG_EXAMPLE_SMOOTH
  #define SENSOR_ANALOG_EXAMPLE_SMOOTH 0
#endif

//TODO-23 for power management will rethink individual timers
#ifndef SENSOR_ANALOG_EXAMPLE_MS
  #define SENSOR_ANALOG_EXAMPLE_MS 15000 // How often to read in MS - default to 15 seconds if not overridden
#endif

#define SENSOR_ANALOG_EXAMPLE_ADVERTISEMENT "\n  -\n    topic: " SENSOR_ANALOG_EXAMPLE_TOPIC "\n    name: " SENSOR_ANALOG_EXAMPLE_NAME "\n    type: int\n    display: bar\n    min: 0\n    max: 6000\n    color: purple\n    rw: r"

#endif // SENSOR_ANALOG_EXAMPLE_H
