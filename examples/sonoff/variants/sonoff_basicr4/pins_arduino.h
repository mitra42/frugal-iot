// This file is destined to be submitted as a variant file once tested
// Note this file is intended to be compatible with ~/.platformio/packages/framework-arduinoespressif8266/variants/itead/pins_arduino.h
// so that code can work across both Sonoff Basic R2 and Basic R4 

#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

static const uint8_t TX = 21;
static const uint8_t RX = 20;

static const uint8_t BUILTIN_BUTTON = 9;
static const uint8_t BUILTIN_RELAY = 4;

#define BUTTON_BUILTIN (9)
#define LED_BUILTIN    (6)
#define RELAY_BUILTIN  (4)

#define BUTTON 9

#endif /* Pins_Arduino_h */