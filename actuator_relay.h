#ifndef ACTUATOR_RELAY_H
#define ACTUATOR_RELAY_H

/* Configuration options
 * Optional: ACTUATOR_RELAY_PIN 
*/
#include "actuator_digital.h"

#define ACTUATOR_RELAY_ADVERTISEMENT "\n  -\n    topic: relay\n    name: Relay\n    type: bool\n    display: toggle\n    rw: rw"

#ifndef ACTUATOR_RELAY_PIN
  //TO_ADD_BOARD
  #ifdef ESP8266_D1_MINI
    // Reasonable to assume using the Wemos Relay shield
    #define ACTUATOR_RELAY_PIN D1 // Default on Wemos relay shield for D1 mini or C3 Pico 
  #else 
    #ifdef LOLIN_C3_PICO
      // Reasonable to assume using the Wemos Relay shield
      #define ACTUATOR_RELAY_PIN 10 // Not sure if pin 10 is called "10" in digital write
    #else
      #ifdef ITEAD_SONOFF
        #define ACTUATOR_RELAY_PIN 12
      #else
        #error Need to define ACTUATOR_RELAY_PIN for unknown boards
      #endif
    #endif
  #endif
#endif // ACTUATOR_RELAY_PIN

#endif // ACTUATOR_RELAY_H