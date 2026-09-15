#ifndef ACTUATOR_DIGITAL_H
#define ACTUATOR_DIGITAL_H

#include "actuator/actuator.h" // Superclass

/* TODO-SLEEP an actuator does not survive deep sleep, and nothing currently tells it about sleep
 * at all.
 *
 * Two separate gaps, found while writing the low-voltage sleep for the OSPIT charge controller:
 *
 * 1. System_Power::prepare() and recover() call frugal_iot.sensors->prepare()/recover() and
 *    nothing else. Actuators are never in the sleep lifecycle, which is why this has never shown
 *    up - sensors are the only group that hears about it.
 *
 * 2. On ESP32 a GPIO is RELEASED during DEEP sleep - it stops being driven and floats, or follows
 *    whatever pull the board has on it - unless it is an RTC-capable pad AND gpio_hold_en(pin) is
 *    called, with gpio_deep_sleep_hold_en() to make holds survive the sleep itself. So a relay can
 *    drop out, or worse be pulled the other way: the FF-OpenMPPT board configures its load switch
 *    with PULL_UP, so a node sleeping to save power could switch its load back ON.
 *
 *    LIGHT sleep does not have this problem - the digital domain stays powered and output states
 *    are retained - which makes Power_Light the better fit for a node with actuators, and is worth
 *    saying in the docs as well as fixing here.
 *
 * What a fix has to cover:
 *   - prepare(): hold the pin, if this pin can be held on this chip. The RTC-capable set differs
 *     between ESP32, S2, S3 and C3, and a non-RTC pin CANNOT be held through deep sleep at all -
 *     so the honest answer for those is to say so at compile time rather than appear to work.
 *   - recover() AND setup(): gpio_hold_dis(pin) before writing, or the write is silently ignored
 *     while the hold is in place. Deep sleep exits through setup(), not recover().
 *   - decide what an actuator SHOULD do when its pin cannot be held: hold the last state is
 *     usually wrong for a valve and right for a light, so it probably belongs to the subclass.
 *
 * Not urgent for any node that never deep sleeps, which is every node with actuators today.
 */
class Actuator_Digital : public Actuator {
  public: 
    Actuator_Digital(const char * const id, const char * const name, const uint8_t pin, const char* color);
  protected:
    uint8_t pin;
    INbool* input;
    void setDefaultColor(char* color);
    void act() override;
    virtual void set(const bool v);
    virtual void setup() override;
    void captiveLines(AsyncResponseStream* response);
}; // Class Actuator_Digital

#endif // ACTUATOR_DIGITAL_H