/* Frugal IoT - Actuator_Analog - a value out as a voltage
 *
 * Set it a voltage and it produces one, on whatever the chip has:
 *
 *   ESP32, ESP32-S2   the built-in DAC        8 bit, GPIO 25/26 (ESP32) or 17/18 (S2)
 *   ESP32-C3, -S3     PWM, via ledc           configurable, any pin
 *   ESP8266           PWM, via analogWrite    configurable, any pin
 *
 * The choice is automatic: SOC_DAC_SUPPORTED says whether the chip has a DAC, so the same sketch
 * compiles for an ESP32 and a C3 and asks for the same volts on each. ACTUATOR_ANALOG_FORCE_PWM
 * overrides it, which is needed for two real cases - the DAC pins are fixed, so any other pin on
 * an ESP32 has to be PWM, and PWM offers finer resolution than 8 bits if that matters more than
 * the filter does.
 *
 * ---------------------------------------------------------------------------------------------
 * THE PWM PATH IS NOT A VOLTAGE WITHOUT AN RC FILTER.
 *
 * It is a square wave whose *average* is the value asked for and whose instantaneous value is
 * either 0 or Vdd - never the number you set. Feeding that straight into something expecting a
 * control voltage will not do what the DAC would. A series R into a capacitor to ground is
 * usually enough; the higher ACTUATOR_ANALOG_PWM_FREQ is, the smaller they can be.
 * ---------------------------------------------------------------------------------------------
 *
 * Volts are nominal on both paths. The DAC is ratiometric to Vdd, so a 3.3V rail reading 3.26
 * shifts everything by that much; the PWM path additionally depends on the filter and on what
 * loads it. Anything needing better than a few percent wants measuring, not trusting.
 *
 * The resolution differs by design rather than being flattened, so each chip gives its best:
 * steps() and stepVolts() report it. That matters to a control loop - an MPPT tracker stepping
 * one LSB at a time moves ~13mV on an ESP32's 8-bit DAC and ~3mV at 10-bit PWM - so a loop with a
 * deadband should ask rather than assume.
 *
 * Build flags:
 *   ACTUATOR_ANALOG_VREF      (3.3)   Full-scale voltage, i.e. the supply the output swings to
 *   ACTUATOR_ANALOG_FORCE_PWM         Use PWM even on a chip that has a DAC
 *   ACTUATOR_ANALOG_PWM_FREQ  (20000) Above audio, so no inductor in the circuit sings
 *   ACTUATOR_ANALOG_PWM_BITS  (10)    ESP32 ties these together: f_max = 80MHz / 2^bits, so
 *                                     12-bit caps at ~19.5kHz. Raising bits means lowering freq,
 *                                     which means a bigger filter.
 *   ACTUATOR_ANALOG_DEBUG
 */

#ifndef ACTUATOR_ANALOG_H
#define ACTUATOR_ANALOG_H

#include "_settings.h"
#include "actuator/actuator.h" // Superclass

#ifndef ACTUATOR_ANALOG_VREF
  #define ACTUATOR_ANALOG_VREF 3.3
#endif
#ifndef ACTUATOR_ANALOG_PWM_FREQ
  #define ACTUATOR_ANALOG_PWM_FREQ 20000
#endif
#ifndef ACTUATOR_ANALOG_PWM_BITS
  #define ACTUATOR_ANALOG_PWM_BITS 10
#endif

// Which way out. SOC_DAC_SUPPORTED is 1 on ESP32 and ESP32-S2, and undefined on C3 and S3.
#if defined(SOC_DAC_SUPPORTED) && SOC_DAC_SUPPORTED && !defined(ACTUATOR_ANALOG_FORCE_PWM)
  #define ACTUATOR_ANALOG_IS_DAC
  #define ACTUATOR_ANALOG_STEPS 256
#elif defined(ESP8266) || defined(ESP32)
  #define ACTUATOR_ANALOG_IS_PWM
  #define ACTUATOR_ANALOG_STEPS (1 << ACTUATOR_ANALOG_PWM_BITS)
#else
  #error Actuator_Analog does not know how to make a voltage on this chip
#endif

class Actuator_Analog : public Actuator {
  public:
    Actuator_Analog(const char * const id, const char * const name, const uint8_t pin,
      const char* color, const float vref = ACTUATOR_ANALOG_VREF);
    // What this hardware can actually do - ask, do not assume. See the note above on control loops.
    uint16_t steps() const { return ACTUATOR_ANALOG_STEPS; }
    float stepVolts() const { return vref / (ACTUATOR_ANALOG_STEPS - 1); }
  protected:
    uint8_t pin;
    float vref;
    INfloat* input; // Volts, wireable so a control can drive it
    void act() override;
    void setup() override;
}; // Class Actuator_Analog

#endif // ACTUATOR_ANALOG_H
