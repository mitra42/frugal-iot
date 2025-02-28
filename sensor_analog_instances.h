#ifndef SENSOR_ANALOG_INSTANCES_H
#define SENSOR_ANALOG_INSTANCES_H

/* Configuration options
 * Required: SENSOR_ANALOG_INSTANCES_PIN_x (where x = 1..5)
 * Optional: SENSOR_ANALOG_MS_x SENSOR_ANALOG_REFERENCE_x SENSOR_ANALOG_SMOOTH_x SENSOR_ANALOG_NAME_x SENSOR_ANALOG_TOPIC_x
*/
//TODO add to this for instances 2..5 if required

#ifdef SENSOR_ANALOG_INSTANCES_WANT

#include "sensor_analog.h"

#ifndef SENSOR_ANALOG_TOPIC_1
  #define SENSOR_ANALOG_TOPIC_1 "analog1"
#endif
#ifndef SENSOR_ANALOG_NAME_1
  #define SENSOR_ANALOG_NAME_1 "Analog1"
#endif

// TO_ADD_BOARD
#ifndef SENSOR_ANALOG_PIN_1
  #ifdef ESP8266_D1
    // Only one analog pin on D1 Mini
    #define SENSOR_ANALOG_PIN_1 A0
  #elif defined(LOLIN_C3_PICO)
      // 0,1,4 work 5 gets error message; 3 is Vbatt; 2 just gets 4095, 8,10 get 0, 7 gets 0 and seems connected to LED
      #error You must define which pins on a Lolin_C3_Pico , typically 0,1,4
  #else
      #error Sorry no default Analog pin for your board // #TO_ADD_BOARD
  #endif
#endif

#ifndef SENSOR_ANALOG_SMOOTH_1
  #define SENSOR_ANALOG_SMOOTH_1 0
#endif

//TODO-23 for power management will rethink individual timers
#ifndef SENSOR_ANALOG_MS_1
  #define SENSOR_ANALOG_MS_1 900000 // How often to read in MS - default to 15 minutes if not overridden
#endif

#define SENSOR_ANALOG_ADVERTISEMENT_1 "\n  -\n    topic: " SENSOR_ANALOG_TOPIC_1 "\n    name: " SENSOR_ANALOG_NAME_1 "\n    type: int\n    display: bar\n    min: 0\n    max: 6000\n    color: purple\n    rw: r"

#endif // SENSOR_ANALOG_INSTANCES_WANT
#endif // SENSOR_ANALOG_INSTANCES_H
