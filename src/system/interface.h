// Deep Sleep issues: none of this survives a deep sleep, which is correct - the objects are rebuilt and `initialized` starts false, so every bus is begun again on the way up.
/* Frugal-IoT - System_Interface - a shared bus, and the power that feeds it
 *
 * The common base of the bus classes: System_I2C_Bus (system/i2c.h), System_OneWire
 * (system/onewire.h) and System_RS485 (system/modbus.h). One object per PHYSICAL bus, shared by
 * every device on it - which is what makes it the right place to put power.
 *
 * Why power belongs to the bus rather than to each sensor
 * -------------------------------------------------------
 * A switched rail feeding an I2C sensor feeds the bus pull-up resistors too - they are on the
 * same breakout. A 1-Wire probe's 4.7k pull-up is the same story. So "power the sensor down"
 * and "power the bus down" are the same wire, and doing it per sensor gets it wrong in both
 * directions:
 *
 *  - two sensors on one bus: the first to be powered down kills the bus under the second
 *  - the bus itself has to be begun AGAIN after the power comes back, and a sensor powering
 *    its own pin has no way to say so
 *
 * So a sensor that has a bus hands its powerPins() straight to that bus - see
 * System_SensorActuator::powerInterface() in system/base.h - and System_Power drives the buses
 * as a separate pass, outside the pass over the sensors: buses up BEFORE the sensors on them and
 * down AFTER them, with the whole-node SYSTEM_POWER3v3_PIN outside that again. See
 * System_Power::prepare()/recover() in system/power.cpp for that ordering.
 *
 * Build flags - each bus class defines its own defaults for these, all PIN_NONE ("not wired"):
 *   SYSTEM_I2C_POWER3v3_PIN     / SYSTEM_I2C_POWER0_PIN
 *   SYSTEM_ONEWIRE_POWER3v3_PIN / SYSTEM_ONEWIRE_POWER0_PIN
 *   SYSTEM_RS485_POWER3v3_PIN   / SYSTEM_RS485_POWER0_PIN
 *   SYSTEM_POWER3v3_PIN         / SYSTEM_POWER0_PIN        - the whole node, see system/power.h
 */
#ifndef SYSTEM_INTERFACE_H
#define SYSTEM_INTERFACE_H

#include <Arduino.h>
#include <vector>
#include "_settings.h" // For PIN_NONE

class System_Interface {
  public:
    System_Interface();
    virtual ~System_Interface() = default;

    /* The pins that switch power to this bus and to everything on it. Either may be PIN_NONE.
     *
     * Leaves them OFF - powerUp() below is what turns them on - matching
     * System_SensorActuator::powerPins(), which has always just claimed the pins.
     */
    void powerPins(uint8_t power3v3, uint8_t power0v);

    // True if a pin was actually driven, i.e. the caller should let the rail settle.
    virtual bool powerUp();
    /* Release the power pins, and note that whatever begin() configured has been lost with them.
     *
     * Does nothing at all on a bus with no power pins - which matters beyond saving two register
     * writes: it is what stops initializeAll() re-scanning a 1-Wire bus that never went away.
     */
    virtual bool powerDown();
    // Bring the bus up. Idempotent - every device on it calls this from its own setup(), and
    // initializeAll() calls it again after the bus has been through a power cycle.
    virtual void initialize() = 0;

    /* The same three, over every bus in the node - what System_Power and System_Frugal call.
     *
     * powerUpAll() returns true if anything was switched on, so the caller can decide whether
     * SYSTEM_POWER_ON_DELAY is worth paying. initializeAll() is separate from powerUpAll()
     * precisely so that delay can go between them: begin() on a bus whose pull-ups are still
     * coming up is how a scan finds nothing.
     */
    static bool powerUpAll();
    static bool powerDownAll();
    static void initializeAll();

  protected:
    uint8_t power3v3_ = PIN_NONE;
    uint8_t power0v_ = PIN_NONE;
    bool initialized = false; // Cleared by powerDown() when the bus loses power

  private:
    // Function-local static, not a file static: a bus can be constructed during static
    // initialization - a sketch with a global System_RS485, or a sensor object that is not
    // built inside setup() - and this way the list is guaranteed to exist before it is used.
    static std::vector<System_Interface*>& all();
};

#endif // SYSTEM_INTERFACE_H
