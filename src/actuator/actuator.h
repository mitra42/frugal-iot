// Deep Sleep issues: the pin is released unless preserveDuringSleep is set - which it is by default. See below.
#ifndef ACTUATOR_H
#define ACTUATOR_H

#include <Arduino.h>
#include "system/base.h"
#include "system/io.h"

/* Note System_SensorActuator, not System_Base: an actuator is a piece of hardware and may sit on
 * a switched rail, exactly as a sensor does. This is what makes powerPins() on an actuator do
 * anything at all, and what makes Actuator_LCD::powerInterface() reachable so its I2C bus can be
 * switched. See the note on System_SensorActuator in system/base.h.
 *
 * setup() powers the device up, as Sensor::setup() does. prepare()/recover() deliberately do NOT
 * power it down and back up the way Sensor::prepare()/recover() do - see the note on those below.
 */
class Actuator : public System_SensorActuator {
  public:
    /* Should this output hold its state through a DEEP sleep?
     *
     * An ESP32 releases its pins during deep sleep unless they are explicitly held, so an output
     * that is not preserved goes wherever the board's pull resistors take it. That is rarely what
     * anyone wants and is hard to predict from the software side, so this defaults to TRUE: the
     * output stays as it was.
     *
     * The responsibility that comes with preserving is choosing the sleep interval. A valve that
     * might need shutting within seconds should not be behind a ten-minute deep sleep at all -
     * use Power_Light, which keeps the digital domain powered and needs none of this. A valve
     * filling a tank over an hour, with plenty of headroom, is perfectly happy with five minutes.
     *
     * Turn it off for an output where "unknown" is safer than "as it was", or where the board has
     * a pull that already puts the pin somewhere sensible:
     *
     *     frugal_iot.actuators->add((new Actuator_Digital("valve1", ...))->preserveDuringSleep(false));
     *
     * Light sleep is unaffected either way - the pins are never released.
     */
    Actuator* preserveDuringSleep(bool on = true) override;
  protected:
    bool preserve_during_sleep = true;
    // An Actuator has a group of inputs used to control it. Some things (like dispatch) will loop through them.
    std::vector<IN*> inputs; // Vector of inputs
    void statusLines(Print* out, bool full) override; // One block per input
    //Actuator();
    Actuator(const char * const id, const char * const name);
    /* Deliberately NOT overridden to powerDown()/powerUp() the way Sensor's are.
     *
     * Cutting an actuator's supply for the sleep contradicts preserveDuringSleep, which defaults
     * to true and is the whole reason Actuator_Digital holds its pin: the hold would freeze a GPIO
     * whose load has no power behind it. Which of the two should win is a decision about the
     * hardware rather than about the code - a valve that must stay open needs its supply, a relay
     * board on a battery node wants the supply gone - so neither is assumed here. An actuator that
     * wants the sleep half overrides prepare()/recover() itself and calls powerDown()/powerUp().
     *
     * void prepare() override;
     * void recover() override;
     */
    void discover() override;
    void dispatch(System_Message &msg) override;
    void setup();
    virtual void act();
}; // Class Actuator

#endif // ACTUATOR_H