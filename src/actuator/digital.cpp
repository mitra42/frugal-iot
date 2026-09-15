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
#include "actuator/actuator.h"
#include "actuator/digital.h" // defines ACUATOR_DIGITAL_DEBUG
#include "system/frugal.h" // for frugal_iot
#ifdef ESP32
  #include "driver/gpio.h"   // gpio_hold_en / gpio_hold_dis
  #include "driver/rtc_io.h" // rtc_gpio_is_valid_gpio, for the warning in setup()
#endif

Actuator_Digital::Actuator_Digital(const char * const id, const char * const name, const uint8_t pin, const char* color)
: Actuator(id, name), 
  pin(pin),
  input(new INbool(id, "on", "On", false, color, false))
{ 
  inputs.push_back(input);
};

void Actuator_Digital::setDefaultColor(char* color) {
  input->default_color = color;
}

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
  act(); // Note act() will also be called if reads any values from file and 'sets' them
}
#pragma GCC diagnostic pop

void Actuator_Digital::setup() {
  Actuator::setup(); // Read config AFTER setup inputs
  #ifdef ESP32
    /* Release any hold left by a deep sleep BEFORE touching the pin.
     *
     * Waking from deep sleep is a reboot, so recover() never runs and this is the only place the
     * hold gets dropped. While it is in place pinMode() and digitalWrite() are silently ignored -
     * the pin keeps its old value and nothing reports a problem, which is a memorable afternoon.
     * Harmless when no hold was set.
     */
    gpio_hold_dis((gpio_num_t)pin);
    if (preserve_during_sleep && !rtc_gpio_is_valid_gpio((gpio_num_t)pin)) {
      // Not fatal - it may still hold on this chip - but it is the case to check on real hardware
      Serial.print(id);
      Serial.print(F(": pin ")); Serial.print(pin);
      Serial.println(F(" is not an RTC pad, so holding it through deep sleep may not work"));
    }
  #endif
  // initialize the digital pin as an output.
  pinMode(pin, OUTPUT);  // Set pin after reading config as may change
  act(); // Set the digital output to match initial conditions.
}

/* Before any sleep. Holding is harmless for a light sleep - the state would have survived anyway -
 * and doing it unconditionally avoids having to work out here which kind of sleep is coming, which
 * is not reliably knowable: System_Power::checkLevel() forces a deep sleep whatever the mode says.
 */
void Actuator_Digital::prepare() {
  #ifdef ESP32
    if (preserve_during_sleep) {
      gpio_hold_en((gpio_num_t)pin);
    }
  #endif
}

// After a LIGHT sleep, which returns to where it slept. A deep sleep goes through setup() instead.
void Actuator_Digital::recover() {
  #ifdef ESP32
    gpio_hold_dis((gpio_num_t)pin);
  #endif
  act(); // Re-assert, in case anything moved the pin while we were not looking
}

void Actuator_Digital::captiveLines(AsyncResponseStream* response) {
  frugal_iot.captive->addBool(response, id, input->id, input->value, name); // Name should be local, doesnt need translating
}
