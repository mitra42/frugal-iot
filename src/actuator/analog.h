// Deep Sleep issues: OPEN QUESTION - a DAC is not a GPIO and is not held. See the note below.
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
 * control voltage will not do what the DAC would.
 *
 * The filter is a resistor in series with the pin and a capacitor from there to ground, with the
 * output taken across the capacitor. Worst-case ripple is at 50% duty:
 *
 *     Vripple (peak-to-peak)  ~=  Vdd / (4 * f * R * C)
 *
 * so for ripple under one step of the output, R*C >= (steps-1) / (4*f), and settling to within a
 * step after a change takes about R*C * bits * 0.7. Values that work, rather than a formula to
 * guess from - at the default 20kHz and 10 bits, where one step is 3.2mV of a 3.3V rail:
 *
 *     R = 10k,  C = 1uF     ripple 4.1mV (1.3 steps), settles 70ms, output impedance 10k
 *     R = 4.7k, C = 2.2uF   ripple 4.0mV,             settles 72ms, output impedance 4.7k
 *
 * Raising ACTUATOR_ANALOG_PWM_FREQ shrinks the capacitor in proportion. At 78kHz, the most that
 * 10-bit allows on an ESP32 (f_max = 80MHz / 2^bits):
 *
 *     R = 10k,  C = 220nF   ripple 4.8mV, settles 15ms, output impedance 10k
 *     R = 1k,   C = 2.2uF   ripple 4.8mV, settles 15ms, output impedance 1k
 *
 * At 8 bits the requirement is four times easier - R*C >= 3.2ms at 20kHz, so 10k and 330nF.
 * At 12 bits it is four times harder - R*C >= 51ms at 20kHz, i.e. 10k and 5.6uF, settling in
 * 0.4s. Resolution is not free: every extra bit costs four times the RC, hence four times the
 * settling time.
 *
 * WATCH THE OUTPUT IMPEDANCE - this is the part that catches people. R sits in series with
 * whatever is being driven, so any current the load draws appears as an error across it:
 *
 *     R = 10k   drawing  10uA -> 100mV out    drawing 100uA -> 1.0V out
 *     R = 1k    drawing  10uA ->  10mV out    drawing 100uA -> 100mV out
 *
 * Into an op-amp or comparator input drawing nanoamps that is nothing. Into anything that loads
 * it, it is the dominant error and no amount of filtering helps. Either take the low-R/high-C
 * pairing above, or buffer the filter with an op-amp follower.
 *
 * Use a ceramic (X7R or better) or film capacitor: an electrolytic's own leakage is a load in
 * exactly the sense above, and its tolerance is wide enough to move the ripple noticeably.
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

/* NOTE deep sleep and this output are an open question.
 *
 * Actuator_Digital holds its pin through a deep sleep (see Actuator::preserveDuringSleep), but a
 * DAC is not a GPIO and gpio_hold_en does not describe it. On ESP32 the DAC is an RTC peripheral,
 * so its output may well persist - but "may well" is not something to build a battery charger on,
 * and the consequence of being wrong is a charge controller whose setpoint goes somewhere unknown
 * while the node sleeps. preserveDuringSleep() is accepted here and currently does nothing.
 *
 * It is question 10 in frugal-iot-irrigation's HARDWARE-QUESTIONS.md: measure the panel voltage
 * while the board is asleep and see whether charging carries on, stops, or changes.
 */
class Actuator_Analog : public Actuator {
  public:
    /* vref is the voltage the output reaches at full scale: the chip's supply rail, so 3.3 on
     * everything here, which is the default. It does not change what the hardware does - it is
     * the scale between the number set and the steps written - but there are two reasons to pass
     * something else:
     *
     *   Accuracy. Both paths are ratiometric to the actual rail, so on a board whose 3.3V rail
     *   measures 3.26, asking for 1.65 gets 1.63. Passing the measured value corrects that.
     *
     *   A gain stage. If the pin feeds an amplifier, pass the full-scale voltage at ITS output,
     *   and the value then means volts where they matter rather than volts at the pin.
     *
     * The input's range follows vref, so the UX slider spans the right values either way.
     */
    Actuator_Analog(const char * const id, const char * const name, const uint8_t pin,
      const char* color = DEFAULT_analog_volts_color, const float vref = ACTUATOR_ANALOG_VREF);
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
