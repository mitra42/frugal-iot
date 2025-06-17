#ifndef ACTUATOR_RELAY_H
#define ACTUATOR_RELAY_H

/* Configuration options
 * Optional: ACTUATOR_RELAY_PIN 
*/
#include "_settings.h"
#include "actuator_digital.h"


#if defined(ACTUATOR_RELAY_WANT) && !defined(ACTUATOR_RELAY_PIN)
  //TO_ADD_BOARD
  #if defined(ITEAD_SONOFF)
        #define ACTUATOR_RELAY_PIN 12
  #elif defined(ESP8266_D1)
    // Reasonable to assume using the Wemos Relay shield
    #define ACTUATOR_RELAY_PIN D1 // Default on Wemos relay shield for D1 mini or C3 Pico 
  #elif defined (LOLIN_C3_PICO) || defined (LOLIN_S2_MINI)
    // Reasonable to assume using the Wemos Relay shield
    #define ACTUATOR_RELAY_PIN 10 // Not sure if pin 10 is called "10" in digital write
  #else
    #error Need to define ACTUATOR_RELAY_PIN for unknown boards
  #endif
#endif // ACTUATOR_RELAY_PIN

#endif // ACTUATOR_RELAY_H