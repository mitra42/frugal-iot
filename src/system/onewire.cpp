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
    scan();
  }
}

/* Walk the bus and remember what is on it.
 *
 * It is begin() that does the walking - getDeviceCount() only hands back the number begin()
 * cached, so re-reading it without another begin() would return the same answer forever.
 *
 * Separate from initialize() because a scan that finds nothing is not necessarily the final
 * answer - the probe may be on switched power that is not up yet, or simply not plugged in yet -
 * and that count is what everything else is driven from: resolveUnbound() has nothing to match
 * against when it is zero, and the captive portal has nothing to list. See rescanIfEmpty().
 */
void System_OneWire::scan() {
  dallas.begin();
  devices = dallas.getDeviceCount();
  if (devices > 0) {
    // Here rather than in initialize(): setResolution() walks the bus writing to each device it
    // finds, so on an empty bus it writes nothing, and a probe that appeared later would be left
    // at whatever resolution it powered up with.
    dallas.setResolution(SYSTEM_ONEWIRE_RESOLUTION);
  }
  #ifdef SYSTEM_ONEWIRE_DEBUG
    Serial.print(F("OneWire pin ")); Serial.print(pin);
    Serial.print(F(": ")); Serial.print(devices); Serial.println(F(" device(s)"));
    // Worth printing: a probe with VDD unconnected powers itself off the data line through the
    // pull-up, which is a different and much more marginal way to run a conversion.
    if (dallas.isParasitePowerMode()) { Serial.println(F("  parasite powered - VDD is not connected")); }
    uint8_t a[SYSTEM_ONEWIRE_ADDRLEN];
    for (uint8_t i = 0; i < devices; i++) {
      if (addressAt(i, a)) { Serial.print(F("  ")); Serial.println(addressToString(a)); }
    }
  #endif
}

/* Free on a bus that has found something, which is the case that matters - one comparison.
 *
 * On a bus that is still empty it costs a whole begin(), and begin() retries three times with a
 * 50ms settle before giving up, so roughly 150ms. That is the price of a node whose probe is not
 * plugged in, paid once per read cycle, and it buys the node that IS plugged in but was not ready
 * at setup() - which is the more common of the two, and the one that used to fail permanently.
 */
void System_OneWire::rescanIfEmpty() {
  if (devices == 0) {
    scan();
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
  rescanIfEmpty(); // The probe may have been powered up, or plugged in, since initialize()
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

/* Read this device's scratchpad, converting first if the last conversion is stale.
 *
 * Retried once, with a fresh conversion rather than just a fresh read, because the FIRST
 * conversion after power is applied to a probe regularly fails and every one after it is fine.
 * The version of this code before the bus was split out did a throwaway requestTemperatures() in
 * setup(), with a comment saying it was needed "to reset OneWire which seems to fail otherwise";
 * the split dropped it as redundant, since the first real read converts anyway - which is true,
 * and missed that the point was that the first conversion is the one that gets thrown away.
 *
 * Doing it here rather than back in setup() costs nothing on a bus that is answering, instead of
 * 750ms of every boot and every deep-sleep wake, and it also covers a probe that recovers later
 * in the life of the node rather than only at setup.
 */
float System_OneWire::tempC(const uint8_t* addr) {
  requestIfDue();
  float c = dallas.getTempC(addr); // DEVICE_DISCONNECTED_C (-127) if it did not answer
  if (c == DEVICE_DISCONNECTED_C) {
    converted = false; // Force requestIfDue() to convert again rather than re-read the same scratchpad
    requestIfDue();
    c = dallas.getTempC(addr);
    #ifdef SYSTEM_ONEWIRE_DEBUG
      Serial.print(F("OneWire pin ")); Serial.print(pin);
      Serial.print(F(": ")); Serial.print(addressToString(addr));
      Serial.print(F(" did not answer the first conversion, retried: ")); Serial.println(c);
    #endif
  }
  return c;
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
