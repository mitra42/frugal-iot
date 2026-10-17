// Deep Sleep issues: none of its own - the expander chip holds the output, provided the chip stays powered. See system/tca9534.h.
/* Frugal IoT - Actuator_Digital_Expander - a digital output on one pin of a TCA9534 I2C expander
 *
 * Everything Actuator_Digital is - an "on" input, the portal checkbox, the MQTT topic, the
 * lifecycle - with the GPIO replaced by one bit of a System_TCA9534. Several of these can share a
 * chip: each names the same address and its own bit, and System_TCA9534::forAddress() hands them
 * all the same object.
 *
 *   frugal_iot.actuators->add(new Actuator_Digital_Expander("valve1", "Valve 1", 0x20, 0, "blue"));
 *
 * `invert` is for a load that is on when its pin is LOW - common on relay boards. It is applied
 * only where the bit is written, so "on" still means on everywhere else: the input, the portal,
 * MQTT, and the value restored from LittleFS. At a cold boot each pin is driven to OFF, whichever
 * level that is, before it becomes an output.
 *
 * Sleep: there is no gpio_hold here because there is no GPIO. The chip itself keeps the output
 * through any ESP32 sleep, light or deep, as long as IT stays powered - which is a matter of
 * which rail it is on, not of code. preserveDuringSleep(false) therefore has nothing to release;
 * an output that should drop during sleep needs a control to turn it off first.
 */
#ifndef ACTUATOR_DIGITAL_EXPANDER_H
#define ACTUATOR_DIGITAL_EXPANDER_H

#include "actuator/digital.h" // Superclass
#include "system/tca9534.h"

class Actuator_Digital_Expander : public Actuator_Digital {
  public:
    Actuator_Digital_Expander(const char * const id, const char * const name,
      const uint8_t addr, const uint8_t expander_pin, const char* color,
      const bool invert = false, TwoWire* wire = &I2C_WIRE);
  protected:
    System_TCA9534* expander; // Shared with any other actuator on the same chip
    const uint8_t expander_pin; // 0-7, P0-P7 on the chip
    const bool invert;        // True if the load is on when the pin is LOW
    void beginOutput() override;
    void writeOutput(const bool on) override;
    void holdOutput() override;    // Nothing to hold - the chip does it
    void releaseOutput() override; // Nothing to release
    System_Interface* powerInterface() override { return expander->interface.bus(); }
};

#endif // ACTUATOR_DIGITAL_EXPANDER_H
