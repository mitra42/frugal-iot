/* Frugal-IoT - System_Interface - see interface.h for why power lives on the bus */

#include "_settings.h"
#include <Arduino.h>
#include <vector>
#include "system/interface.h"
#include "misc.h" // pinsPowerUp / pinsPowerDown

std::vector<System_Interface*>& System_Interface::all() {
  static std::vector<System_Interface*> interfaces;
  return interfaces;
}

// Every bus registers itself, so System_Power can walk them all without knowing what kinds exist
System_Interface::System_Interface() {
  all().push_back(this);
}

/* Note the warning: only ONE pair of pins can switch a bus, so two devices on it asking for
 * different ones is a wiring or configuration mistake. The second wins and the first's pin is
 * left an unpowered OUTPUT, so the symptom - a bus that reads nothing - says nothing about the
 * cause. Firing it here also catches the other version of the same mistake, a build that sets
 * SYSTEM_I2C_POWER3v3_PIN AND a per-sensor SENSOR_<x>_POWER3v3_PIN that disagrees with it.
 */
void System_Interface::powerPins(const uint8_t power3v3, const uint8_t power0v) {
  if (((power3v3_ != PIN_NONE) && (power3v3_ != power3v3))
   || ((power0v_ != PIN_NONE) && (power0v_ != power0v))) {
    Serial.printf("Bus power pins changed from 3v3=%d 0v=%d to 3v3=%d 0v=%d - only one pair can "
      "switch a bus, check the build flags\n", power3v3_, power0v_, power3v3, power0v);
  }
  power3v3_ = power3v3;
  power0v_ = power0v;
  if (power3v3_ != PIN_NONE) {
    pinMode(power3v3_, OUTPUT);
  }
  if (power0v_ != PIN_NONE) {
    pinMode(power0v_, OUTPUT);
  }
}

bool System_Interface::powerUp() {
  return pinsPowerUp(power3v3_, power0v_);
}

// See the note in interface.h on why an unpowered bus is left entirely alone here
bool System_Interface::powerDown() {
  const bool any = pinsPowerDown(power3v3_, power0v_);
  if (any) {
    initialized = false; // Whatever begin() configured went away with the power
  }
  return any;
}

bool System_Interface::powerUpAll() {
  bool any = false;
  for (System_Interface* i : all()) {
    any |= i->powerUp();
  }
  return any;
}

bool System_Interface::powerDownAll() {
  bool any = false;
  for (System_Interface* i : all()) {
    any |= i->powerDown();
  }
  return any;
}

// A no-op on any bus that did not lose power, because initialize() is idempotent
void System_Interface::initializeAll() {
  for (System_Interface* i : all()) {
    i->initialize();
  }
}
