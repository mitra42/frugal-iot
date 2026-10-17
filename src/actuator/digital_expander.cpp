/* Frugal IoT - Actuator_Digital_Expander - see actuator/digital_expander.h */

#include "_settings.h"
#include "actuator/digital_expander.h"

Actuator_Digital_Expander::Actuator_Digital_Expander(const char * const id, const char * const name,
  const uint8_t addr, const uint8_t expander_pin, const char* color, const bool invert, TwoWire* wire)
: Actuator_Digital(id, name, PIN_NONE, color), // No GPIO - nothing in Actuator_Digital touches `pin` once the hooks are overridden
  expander(System_TCA9534::forAddress(addr, wire)),
  expander_pin(expander_pin),
  invert(invert)
{ }

/* Called before the config is read, as Actuator_Digital's is. Off is the safe state for a cold
 * boot; after a warm reboot or a deep sleep the chip's own state is kept - see configureOutput().
 * Either way Actuator_Digital::setup() then act()s with the restored value.
 */
void Actuator_Digital_Expander::beginOutput() {
  expander->initialize();
  expander->configureOutput(expander_pin, invert); // OFF, as a level: LOW unless inverted
}

void Actuator_Digital_Expander::writeOutput(const bool on) {
  expander->writeBit(expander_pin, on != invert);
}

void Actuator_Digital_Expander::holdOutput() { }

void Actuator_Digital_Expander::releaseOutput() { }
