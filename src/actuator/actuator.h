#ifndef ACTUATOR_H
#define ACTUATOR_H

#include <Arduino.h>
#include "system/base.h"
#include "system/io.h"

class Actuator : public System_Base {
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
    //Actuator();
    Actuator(const char * const id, const char * const name);
    void discover() override;
    void dispatch(System_Message &msg) override;
    void setup();
    virtual void act();
}; // Class Actuator

#endif // ACTUATOR_H