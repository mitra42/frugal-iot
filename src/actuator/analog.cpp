/* Frugal IoT - Actuator_Analog - see analog.h, especially the note about PWM needing a filter */

#include "_settings.h"
#include <Arduino.h>
#include "actuator/actuator.h"
#include "actuator/analog.h"
#include "system/frugal.h" // for frugal_iot

Actuator_Analog::Actuator_Analog(const char * const id, const char * const name, const uint8_t pin,
  const char* color, const float vref)
: Actuator(id, name),
  pin(pin),
  vref(vref),
  // Volts, two decimals - a step is ~13mV on an 8-bit DAC at 3.3V, so a third digit would be
  // claiming precision the hardware does not have. Wireable: driving this from a control is the
  // point of it existing.
  // Range is 0..vref, the hardware's actual span; the schema's 0..3.3 are the defaults, so
  // discover() only reports min/max when a board has passed a vref that is not the plain rail.
  input(new INfloat(id, "volts", "Volts", 0, 2, 0, vref,
    DEFAULT_analog_volts_min, DEFAULT_analog_volts_max, color, true))
{
  inputs.push_back(input);
};

void Actuator_Analog::act() {
  // Clamp rather than wrap: asking for more than the rail can give is a bug in the caller, and
  // silently outputting near-zero because the value wrapped would be a very confusing one.
  float v = input->floatValue();
  if (v < 0) { v = 0; }
  if (v > vref) { v = vref; }
  const uint32_t raw = (uint32_t)((v / vref) * (ACTUATOR_ANALOG_STEPS - 1) + 0.5f); // round, not truncate
  #ifdef ACTUATOR_ANALOG_DEBUG
    Serial.print(F("Actuator_Analog ")); Serial.print(id); Serial.print(F(" = "));
    Serial.print(v, 3); Serial.print(F("V raw=")); Serial.print(raw);
    Serial.print(F("/")); Serial.println(ACTUATOR_ANALOG_STEPS - 1);
  #endif
  #if defined(ACTUATOR_ANALOG_IS_DAC)
    dacWrite(pin, (uint8_t)raw);
  #elif defined(ESP8266)
    analogWrite(pin, raw);
  #else // ESP32 family without a DAC - ledc, attached in setup()
    ledcWrite(pin, raw);
  #endif
}

void Actuator_Analog::setup() {
  Actuator::setup(); // Read config AFTER setting up inputs, as Actuator_Digital does
  #if defined(ACTUATOR_ANALOG_IS_DAC)
    // dacWrite reports whether this pin actually has a DAC on it - GPIO 25/26 on an ESP32, 17/18
    // on an S2. Any other pin needs ACTUATOR_ANALOG_FORCE_PWM, and saying so at setup beats
    // silently producing nothing.
    if (!dacWrite(pin, 0)) {
      Serial.print(id); Serial.print(F(": pin ")); Serial.print(pin);
      Serial.println(F(" has no DAC - use ACTUATOR_ANALOG_FORCE_PWM, or a DAC pin (25/26, or 17/18 on S2)"));
      setupFailed();
    }
  #elif defined(ESP8266)
    analogWriteFreq(ACTUATOR_ANALOG_PWM_FREQ);
    analogWriteRange(ACTUATOR_ANALOG_STEPS - 1);
    pinMode(pin, OUTPUT);
  #else
    if (!ledcAttach(pin, ACTUATOR_ANALOG_PWM_FREQ, ACTUATOR_ANALOG_PWM_BITS)) {
      // Usually means no ledc channel left, or a frequency the resolution cannot reach:
      // on ESP32 f_max = 80MHz / 2^bits.
      Serial.print(id); Serial.print(F(": could not attach PWM to pin ")); Serial.println(pin);
      setupFailed();
    }
  #endif
  act(); // Drive the output to match the initial value, rather than leaving the pin floating
}
