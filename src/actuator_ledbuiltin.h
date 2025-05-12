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
  #define ACTUATOR_LEDBUILTIN_RGB
#elif defined(SONOFF_R2) || defined(ESP8266_D1) 
  #define ACTUATOR_LEDBUILTIN_INVERT
#elif defined(LOLIN_S2_MINI)
  // Not defined LED is digital and not inverted
#else
  #error "please define whether your LED is RGB or not"
#endif // BOARDS

#ifndef ACTUATOR_LEDBUILTIN_BRIGHTNESS
  #define ACTUATOR_LEDBUILTIN_BRIGHTNESS 255
#endif
#ifndef ACTUATOR_LEDBUILTIN_COLOR
  #define ACTUATOR_LEDBUILTIN_COLOR "0x00FF00" // White
#endif



class Actuator_Ledbuiltin : public Actuator_Digital {
  public: 
    Actuator_Ledbuiltin(const uint8_t p, uint8_t brightness = 255, const char* color = "0xFFFFFF");
    virtual void act();
    uint8_t brightness; // Brightness of LED  0-255
    #ifdef ACTUATOR_LEDBUILTIN_RGB
      INcolor* color;
    #endif
    void dispatchTwig(const String &topicActuatorId, const String &leaf, const String &payload, bool isSet);
};

#endif // ACTUATOR_LEDBUILTIN_H