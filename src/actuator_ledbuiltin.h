#ifndef ACTUATOR_LEDBUILTIN_H
#define ACTUATOR_LEDBUILTIN_H

/* Configuration options
 * Optional: ACTUATOR_LEDBUILTIN_DEBUG ACTUATOR_LEDBUILTIN_PIN - defaults
 * Optional: ACTUATOR_LEDBUILTIN_BRIGHTNESS
 * Optional: BUILTIN_LED LED_BUILTIN RGB_BUILTIN - set on various boards  

 * For reference the LED is on the following pins for boards we have been working with .... 
 * Sonoff: 13
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
  #elif defined(BUILTIN_LED)
      #define ACTUATOR_LEDBUILTIN_PIN BUILTIN_LED
  #elif defined(LILYGOHIGROW)
    #define ACTUATOR_LEDBUILTIN_PIN 18
  #else 
    #error "No ACTUATOR_LEDBUILTIN_PIN pin defined and no default known for this board"
  #endif
#endif // ACTUATOR_LEDBUILTIN_PIN

// TODO-ADD-BOARD - add your board here
#if defined(LOLIN_C3_PICO) || defined(LILYGOHIGROW)
  #define ACTUATOR_LEDBUILTIN_RGB true
#elif defined(SONOFF_R2) || defined(ESP8266_D1)
  #define ACTUATOR_LEDBUILTIN_RGB false
#else
  #error "please define whether your LED is RGB or not"
#endif // BOARDS

#ifndef ACTUATOR_LEDBUILTIN_BRIGHTNESS
  #define ACTUATOR_LEDBUILTIN_BRIGHTNESS 255
#endif


class Actuator_Ledbuiltin : public Actuator_Digital {
  public: 
    Actuator_Ledbuiltin(const uint8_t p, const char* topicLeaf, bool rgb = false, uint8_t brightness = 255);
    virtual void act();
    bool rgb; // If true then use RGB values
    uint8_t brightness; // Brightness of LED  0-255
};

#endif // ACTUATOR_LEDBUILTIN_H