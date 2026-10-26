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
  #include "soc/soc_caps.h"  // SOC_RTCIO_HOLD_SUPPORTED - which chips hold via RTC pads
  #include "driver/gpio.h"   // gpio_hold_en / gpio_hold_dis
  #if defined(SOC_RTCIO_HOLD_SUPPORTED) && SOC_RTCIO_HOLD_SUPPORTED
    #include "driver/rtc_io.h" // rtc_gpio_is_valid_gpio, for the warning in setup()
  #endif
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
  writeOutput(input->value);
}

void Actuator_Digital::writeOutput(const bool on) {
  digitalWrite(pin, on ? HIGH : LOW); // Relay pin on Wemos shield is NOT inverted
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

/* Make the GPIO an output, releasing any deep-sleep hold first.
 *
 * Called from setup() BEFORE Actuator::setup() reads the config - see the note there.
 */
void Actuator_Digital::beginOutput() {
  #ifdef ESP32
    /* Release any hold left by a deep sleep BEFORE touching the pin.
     *
     * Waking from deep sleep is a reboot, so recover() never runs and this is the only place the
     * hold gets dropped. While it is in place pinMode() and digitalWrite() are silently ignored -
     * the pin keeps its old value and nothing reports a problem, which is a memorable afternoon.
     * Harmless when no hold was set.
     *
     * Skipped when `pin` is not a real GPIO - see holdable().
     */
    if (holdable()) {
      gpio_hold_dis((gpio_num_t)pin);
    }
    #if defined(SOC_RTCIO_HOLD_SUPPORTED) && SOC_RTCIO_HOLD_SUPPORTED
      /* The hold itself works either way. This is about the wake: see digital.h. On a chip that has
       * an RTC IO mux, an RTC pad is the better choice, so a pin outside that set is worth saying.
       *
       * Deliberately NOT done on the C3, which has no RTC IO mux at all (SOC_RTCIO_PIN_COUNT is 0).
       * rtc_gpio_is_valid_gpio() still compiles there but returns false for EVERY pin, so an
       * unguarded check would warn about all of them - on one of the commonest boards here - and
       * about something the user could not fix anyway.
       */
      if (preserve_during_sleep && holdable() && !rtc_gpio_is_valid_gpio((gpio_num_t)pin)) {
        Serial.print(id);
        Serial.print(F(": pin ")); Serial.print(pin);
        Serial.println(F(" is not an RTC pad, so it will float for the first moments after waking"));
      }
    #endif
  #endif
  pinMode(pin, OUTPUT);
}

void Actuator_Digital::setup() {
  /* Initialize the output, BEFORE Actuator::setup() reads the config.
   *
   * Restoring a saved value dispatches it, and Actuator_Digital::set() calls act(), which
   * digitalWrites the pin - see the note in set(). With the config read first, that write
   * landed on a pin the peripheral manager still had as ESP32_BUS_TYPE_INIT, so the ESP32
   * core refused it and logged "IO n is not set as GPIO". Harmless, since the act() below
   * repeated the write, but it put a red error in every boot log of a board with a saved value.
   *
   * The previous ordering was justified with "set pin after reading config as may change",
   * but `pin` is a constructor argument (digital.h) and nothing assigns it from config.
   */
  beginOutput();
  Actuator::setup(); // Read config AFTER setup inputs, and now also after the pin is an output
  act(); // Set the digital output to match initial conditions.
}

/* Before any sleep. Holding is harmless for a light sleep - the state would have survived anyway -
 * and doing it unconditionally avoids having to work out here which kind of sleep is coming, which
 * is not reliably knowable: System_Power::checkLevel() forces a deep sleep whatever the mode says.
 */
void Actuator_Digital::prepare() {
  if (preserve_during_sleep) {
    holdOutput();
  }
}

// After a LIGHT sleep, which returns to where it slept. A deep sleep goes through setup() instead.
void Actuator_Digital::recover() {
  releaseOutput();
  act(); // Re-assert, in case anything moved the pin while we were not looking
}

#ifdef ESP32
/* Can gpio_hold_* be applied to `pin`? Only to a real output-capable GPIO.
 *
 * Not every `pin` is one. On boards with an RGB LED the core defines LED_BUILTIN as
 * SOC_GPIO_PIN_COUNT + the LED's pin (e.g. 29 on a C3 Pico), a virtual number that only
 * digitalWrite() understands and routes to rgbLedWrite(). The IDF rejects it with
 * "gpio_hold_dis(...): Only output-capable GPIO support this function" on every boot. Nothing is
 * lost by skipping it: a WS2812 keeps its colour for as long as it has power, with no hold at all.
 */
bool Actuator_Digital::holdable() {
  return GPIO_IS_VALID_OUTPUT_GPIO(pin);
}
#endif

void Actuator_Digital::holdOutput() {
  #ifdef ESP32
    if (holdable()) {
      gpio_hold_en((gpio_num_t)pin);
    }
  #endif
}

void Actuator_Digital::releaseOutput() {
  #ifdef ESP32
    if (holdable()) {
      gpio_hold_dis((gpio_num_t)pin);
    }
  #endif
}

void Actuator_Digital::captiveLines(AsyncResponseStream* response) {
  frugal_iot.captive->addBool(response, id, input->id, input->value, name); // Name should be local, doesnt need translating
}
