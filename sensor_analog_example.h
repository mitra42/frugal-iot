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
#define SENSOR_ANALOG_EXAMPLE_ADVERTISEMENT "\n  -\n    topic: " SENSOR_ANALOG_EXAMPLE_TOPIC "\n    name: " SENSOR_ANALOG_EXAMPLE_NAME "\n    type: int\n    display: bar\n    min: 0\n    max: 6000\n    color: purple\n    rw: r"

Sensor_Analog sensor_analog_example;

#endif // SENSOR_ANALOG_EXAMPLE_H
