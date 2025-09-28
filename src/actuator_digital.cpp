/* Frugal IoT - Actuator_Digital - a standard digital actuator, e.g. for a relay
 * 
 * The pin is either defined in the subclass, or in the constructor call in main.cpp
 * 
 * Common pins used. 
 *  - ESP8266 D1 shields - relay is usually on D1
 *  - LOLIN C2 Pico or S2 Mini - relay is pin 10 (same pin as D1 on ESP8266 D1)
 *  - ITEAD Sonoff - relay is on pin 12
 * 
 */

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "actuator.h"
#include "actuator_digital.h" // defines ACUATOR_DIGITAL_DEBUG
#include "system_frugal.h" // for frugal_iot

Actuator_Digital::Actuator_Digital(const char * const id, const char * const name, const uint8_t pin, const char* color)
: Actuator(id, name), 
  pin(pin),
  input(new INbool(id, "on", "On", false, color, false))
{ 
  inputs.push_back(input);
};

void Actuator_Digital::act() {
  digitalWrite(pin, input->value ? HIGH : LOW); // Relay pin on Wemos shield is NOT inverted
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void Actuator_Digital::set(const bool v) {
  // Note there is nothing to actually "set"
  #ifdef ACTUATOR_DIGITAL_DEBUG
    Serial.print(F("\nSetting ")); Serial.print(name); Serial.println(v ? F(" on") : F(" off"));
  #endif
  act();
}
#pragma GCC diagnostic pop

void Actuator_Digital::setup() {
  Actuator::setup(); // Read config AFTER setup inputs
  // initialize the digital pin as an output.
  pinMode(pin, OUTPUT);  // Set pin after reading config as may change
  act(); // Set the digital output to match initial conditions.
}

void Actuator_Digital::captiveLines(AsyncResponseStream* response) {
  frugal_iot.captive->addBool(response, id, input->id, input->value, name);
}
