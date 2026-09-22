/* Frugal-IoT - shared OneWire bus - see onewire.h for why this is shared rather than per-sensor */

#include "_settings.h"
#include <Arduino.h>
#include <vector>
#include <string.h> // for memcmp/memcpy
#include "system/onewire.h"

// One bus per pin, for the lifetime of the process. A std::vector rather than a map: a node has
// one or two 1-Wire pins, so a linear scan is smaller and faster than anything cleverer.
static std::vector<System_OneWire*> buses;

System_OneWire* System_OneWire::forPin(uint8_t pin) {
  for (auto b : buses) {
    if (b->pin == pin) {
      return b;
    }
  }
  System_OneWire* b = new System_OneWire(pin);
  buses.push_back(b);
  return b;
}

System_OneWire::System_OneWire(uint8_t pin)
: pin(pin), wire(pin), dallas(&wire) { }

// Idempotent, so every device on the bus can safely call it from its own setup() - the same
// contract as System_I2C::initialize() and System_RS485::initialize().
void System_OneWire::initialize() {
  if (!initialized) {
    initialized = true;
    dallas.begin();
    dallas.setResolution(SYSTEM_ONEWIRE_RESOLUTION);
    devices = dallas.getDeviceCount();
    #ifdef SYSTEM_ONEWIRE_DEBUG
      Serial.print(F("OneWire pin ")); Serial.print(pin);
      Serial.print(F(": ")); Serial.print(devices); Serial.println(F(" device(s)"));
      uint8_t a[SYSTEM_ONEWIRE_ADDRLEN];
      for (uint8_t i = 0; i < devices; i++) {
        if (addressAt(i, a)) { Serial.print(F("  ")); Serial.println(addressToString(a)); }
      }
    #endif
  }
}

uint8_t System_OneWire::count() {
  return devices;
}

void System_OneWire::add(OneWireDevice* device) {
  users.push_back(device);
}

bool System_OneWire::isClaimed(const uint8_t* addr) {
  bool claimed = false;
  for (auto u : users) {
    if (u->owBound() && (memcmp(u->owAddress(), addr, SYSTEM_ONEWIRE_ADDRLEN) == 0)) {
      claimed = true;
    }
  }
  return claimed;
}

// See onewire.h for why the rule is exactly one-to-one and nothing looser.
void System_OneWire::resolveUnbound() {
  OneWireDevice* orphan = nullptr;
  uint8_t orphans = 0;
  for (auto u : users) {
    if (!u->owBound()) {
      orphans++;
      orphan = u;
    }
  }
  if (orphans == 1) {
    uint8_t a[SYSTEM_ONEWIRE_ADDRLEN];
    uint8_t spare[SYSTEM_ONEWIRE_ADDRLEN];
    uint8_t spares = 0;
    for (uint8_t i = 0; i < devices; i++) {
      if (addressAt(i, a) && !isClaimed(a)) {
        spares++;
        memcpy(spare, a, SYSTEM_ONEWIRE_ADDRLEN);
      }
    }
    if (spares == 1) {
      orphan->owBindTo(spare);
      #ifdef SYSTEM_ONEWIRE_DEBUG
        Serial.print(F("OneWire: matched the one unbound sensor to the one unclaimed probe "));
        Serial.println(addressToString(spare));
      #endif
    }
  }
}

bool System_OneWire::addressAt(uint8_t index, uint8_t* addr) {
  return dallas.getAddress(addr, index);
}

bool System_OneWire::isPresent(const uint8_t* addr) {
  return dallas.isConnected(addr);
}

/* Broadcast a convert, but only if one is not already fresh.
 *
 * Several sensors sharing this bus each call tempC() within the same periodically() pass; the
 * first triggers the conversion and blocks for it, and the rest read the scratchpad the same
 * conversion filled. `converted` starts false and the object is rebuilt by the restart that deep
 * sleep really is, so the first read after any boot or wake always converts rather than reading
 * a scratchpad that was never filled.
 */
void System_OneWire::requestIfDue() {
  const unsigned long now = millis();
  if (!converted || ((now - lastConvertMs) >= SYSTEM_ONEWIRE_RECONVERT_MS)) {
    dallas.requestTemperatures(); // Blocks ~750ms at 12-bit - the cost this class exists to share
    lastConvertMs = millis();     // After, not before: the wait is part of the gap we are timing
    converted = true;
  }
}

float System_OneWire::tempC(const uint8_t* addr) {
  requestIfDue();
  return dallas.getTempC(addr); // DEVICE_DISCONNECTED_C (-127) if it did not answer
}

bool System_OneWire::addressFromString(const String& s, uint8_t* addr) {
  bool ok = (s.length() == SYSTEM_ONEWIRE_ADDRSTRLEN);
  if (ok) {
    for (uint8_t i = 0; ok && (i < SYSTEM_ONEWIRE_ADDRLEN); i++) {
      uint8_t byte = 0;
      for (uint8_t nibble = 0; nibble < 2; nibble++) {
        const char c = s.charAt(i * 2 + nibble);
        uint8_t v;
        if      (c >= '0' && c <= '9') { v = c - '0'; }
        else if (c >= 'a' && c <= 'f') { v = c - 'a' + 10; }
        else if (c >= 'A' && c <= 'F') { v = c - 'A' + 10; }
        else { ok = false; v = 0; }
        byte = (byte << 4) | v;
      }
      addr[i] = byte;
    }
  }
  return ok;
}

String System_OneWire::addressToString(const uint8_t* addr) {
  char buf[SYSTEM_ONEWIRE_ADDRSTRLEN + 1];
  for (uint8_t i = 0; i < SYSTEM_ONEWIRE_ADDRLEN; i++) {
    snprintf(buf + (i * 2), 3, "%02x", addr[i]);
  }
  return String(buf);
}
