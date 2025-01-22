#ifndef ACTUATOR_LEDBUILTIN_H
#define ACTUATOR_LEDBUILTIN_H

/* Configuration options
 * Optional: ACTUATOR_LEDBUILTIN_DEBUG ACTUATOR_LEDBUILTIN_PIN - defaults
*/

#include "actuator_digital.h" // for class Actuator_Digital

// TO_ADD_BOARD, look this thru and check it - there are historical bugs in board definitions in this area, so its art rather than science! 
// Arduinos unfortunately dont use any kind of #define for the board type so the 
// standard blinkplay fails on for example the WEMOS boards.
// Arduino_UNO Pin 13 has an LED connected on most Arduino boards but doesnt define BUILTIN_LED
// This next part is to handle some weirdnesses where early versions of ESP8266 define BUILTIN_LED instead of LED_BUILTIN
// but BUILTIN_LED responds to ifndef
// This version works on ESP8266 D1 Mini and ESP32 Lolin C3 Pico and SONOFF R2,
// It is not tested on others but should work if LED_BUILTIN is corectly defined for them. 
// 
#ifndef ACTUATOR_LEDBUILTIN_PIN
  #ifdef LED_BUILTIN
    #define ACTUATOR_LEDBUILTIN_PIN LED_BUILTIN
  #else // !LED_BUILTIN
    #ifdef BUILTIN_LED
      #define ACTUATOR_LEDBUILTIN_PIN BUILTIN_LED
    #else
      #error "No LED pin defined and no default specified"
    #endif // BUILTIN_LED
  #endif // LED_BUILTIN
#endif // ACTUATOR_LEDBUILTIN_PIN

#define ACTUATOR_LEDBUILTIN_ADVERTISEMENT "\n  -\n    topic: ledbuiltin\n    name: Built in LED\n    type: bool\n    color: orange\n    display: toggle\n    rw: rw"


class Actuator_Ledbuiltin : public Actuator_Digital {
  public: 
    Actuator_Ledbuiltin(const uint8_t p, const char* topic);
    virtual void act();
};

namespace aLedbuiltin {
extern Actuator_Ledbuiltin actuator_ledbuiltin;

void setup();
void loop();
} // namespace aLedbuiltin
#endif // ACTUATOR_LEDBUILTIN_H