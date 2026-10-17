/* Frugal-IoT - shared OneWire bus - see onewire.h for why this is shared rather than per-sensor */

#include "_settings.h"
#include <Arduino.h>
#include <vector>
#include <string.h> // for memcmp/memcpy
#include "system/onewire.h"
#include "utils/Placeholder.h" // OneWireNg's - a DSTherm::Scratchpad cannot be default-constructed

// Our ROM ids are plain byte arrays, which is what OneWireNg::Id is underneath
static inline const OneWireNg::Id& asId(const uint8_t* addr) {
  return *reinterpret_cast<const OneWireNg::Id*>(addr);
}

/* One bus per pin, for the lifetime of the process. A std::vector rather than a map: a node has
 * one or two 1-Wire pins, so a linear scan is smaller and faster than anything cleverer.
 *
 * Function-local rather than a file static so that it is guaranteed to exist even if a bus is
 * created during static initialization - see System_I2C_Bus::forWire(), same shape.
 */
static std::vector<System_OneWire*>& onewire_buses() {
  static std::vector<System_OneWire*> buses;
  return buses;
}

System_OneWire* System_OneWire::forPin(uint8_t pin) {
  for (auto b : onewire_buses()) {
    if (b->pin == pin) {
      return b;
    }
  }
  System_OneWire* b = new System_OneWire(pin);
  onewire_buses().push_back(b);
  return b;
}

/* No internal pull-up: the bus has its 4.7k, as it did with Stoffregen's OneWire, which did not
 * enable one either. The ESP32's ~45k would do no harm in parallel, but would also let a bus with
 * no resistor half-work, which is harder to diagnose than one that does not work at all.
 */
System_OneWire::System_OneWire(uint8_t pin)
: pin(pin), ow(pin, false) {
  powerPins(SYSTEM_ONEWIRE_POWER3v3_PIN, SYSTEM_ONEWIRE_POWER0_PIN);
  #if (CONFIG_MAX_SEARCH_FILTERS > 0)
    // Search for thermometers only. Anything else on the bus has nothing to say to a
    // Sensor_DS18B20, and binding one to it would publish garbage as a temperature.
    DSTherm(ow).filterSupportedSlaves();
  #endif
}

// Idempotent, so every device on the bus can safely call it from its own setup() - the same
// contract as System_I2C_Bus::initialize() and System_RS485::initialize().
// powerUp() first for the same reason as System_I2C_Bus::initialize() - a bus reached outside
// the System_Power lifecycle still needs its rail - and because scan() below talks to the probes.
void System_OneWire::initialize() {
  if (!initialized) {
    initialized = true;
    powerUp();
    scan();
  }
}

// See onewire.h - the enumeration does not survive the power going away
bool System_OneWire::powerDown() {
  const bool any = System_Interface::powerDown();
  if (any) {
    devices = 0;
    converted = false; // The scratchpads went with the power, so read nothing back from them
  }
  return any;
}

/* Walk the bus and remember what is on it.
 *
 * The ids are kept, rather than searching again whenever addressAt() is asked - DallasTemperature's
 * getAddress(index) re-walked the bus on every call, so the captive portal listing three probes
 * walked it three times.
 *
 * Separate from initialize() because a scan that finds nothing is not necessarily the final
 * answer - the probe may be on switched power that is not up yet, or simply not plugged in yet -
 * and that count is what everything else is driven from: resolveUnbound() has nothing to match
 * against when it is zero, and the captive portal has nothing to list. See rescanIfEmpty().
 */
void System_OneWire::scan() {
  devices = 0;
  for (const auto& id : ow) { // The range-loop is OneWireNg's search, CRC-checked
    if (devices < SYSTEM_ONEWIRE_MAX_DEVICES) {
      memcpy(ids[devices], id, SYSTEM_ONEWIRE_ADDRLEN);
      devices++;
    }
    #ifdef SYSTEM_ONEWIRE_DEBUG
      else { Serial.println(F("OneWire: more probes than SYSTEM_ONEWIRE_MAX_DEVICES, ignoring the rest")); }
    #endif
  }
  // 0 means some probe is parasitic. Asked every scan, since the probes may have changed.
  parasite = (devices > 0) && (DSTherm(ow).readPowerSupplyAll() == 0);
  /* Here rather than in initialize(), and every scan: a probe that appeared later, or was power
   * cycled, comes up at whatever resolution its EEPROM says. Not copied to the EEPROM - that wears,
   * and this runs again after anything that could have reset it.
   */
  for (uint8_t i = 0; i < devices; i++) {
    setResolution(ids[i]);
  }
  #ifdef SYSTEM_ONEWIRE_DEBUG
    Serial.print(F("OneWire pin ")); Serial.print(pin);
    Serial.print(F(": ")); Serial.print(devices); Serial.println(F(" thermometer(s)"));
    // Worth printing: a probe with VDD unconnected powers itself off the data line through the
    // pull-up, which is a different and much more marginal way to run a conversion.
    if (parasite) { Serial.println(F("  parasite powered - VDD is not connected")); }
    uint8_t a[SYSTEM_ONEWIRE_ADDRLEN];
    for (uint8_t i = 0; i < devices; i++) {
      if (addressAt(i, a)) { Serial.print(F("  ")); Serial.println(addressToString(a)); }
    }
  #endif
}

/* Free on a bus that has found something, which is the case that matters - one comparison.
 *
 * On a bus that is still empty it costs a search, which on an empty bus is one reset with no
 * presence pulse - about a millisecond. (DallasTemperature's begin() retried three times with a
 * 50ms settle, so this used to be ~150ms.) That buys the node whose probe IS plugged in but was not
 * ready at setup() - the more common case, and the one that used to fail permanently.
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
  const bool ok = (index < devices);
  if (ok) {
    memcpy(addr, ids[index], SYSTEM_ONEWIRE_ADDRLEN);
  }
  return ok;
}

// Asks the probe itself rather than the list from the last scan - a CRC-good scratchpad read.
bool System_OneWire::isPresent(const uint8_t* addr) {
  Placeholder<DSTherm::Scratchpad> scrpd;
  return DSTherm(ow).readScratchpad(asId(addr), scrpd) == OneWireNg::EC_SUCCESS;
}

/* Set the resolution in the probe's scratchpad, if it is not already right.
 *
 * Read-modify-write, so the alarm thresholds Th/Tl are left alone. The DS18S20 (family 0x10) has a
 * fixed resolution and no configuration register, so it is skipped.
 */
void System_OneWire::setResolution(const OneWireNg::Id& id) {
  const DSTherm::Resolution want = (DSTherm::Resolution)(SYSTEM_ONEWIRE_RESOLUTION - 9);
  Placeholder<DSTherm::Scratchpad> scrpd;
  if ((id[0] != 0x10) && (DSTherm(ow).readScratchpad(id, scrpd) == OneWireNg::EC_SUCCESS)) {
    DSTherm::Scratchpad& sp = scrpd;
    if (sp.getResolution() != want) {
      sp.setResolution(want);
      sp.writeScratchpad();
    }
  }
}

float System_OneWire::readC(const uint8_t* addr) {
  float c = SYSTEM_ONEWIRE_DISCONNECTED_C;
  Placeholder<DSTherm::Scratchpad> scrpd;
  if (DSTherm(ow).readScratchpad(asId(addr), scrpd) == OneWireNg::EC_SUCCESS) {
    const DSTherm::Scratchpad& sp = scrpd;
    c = sp.getTemp2() / 16.0f; // getTemp2() is sixteenths of a degree - exact, unlike getTemp()'s thousandths
  }
  return c;
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
    /* Blocks up to 750ms at 12-bit - the cost this class exists to share. With every probe
     * powered from VDD, SCAN_BUS polls for completion and returns as soon as they are done; a
     * parasitic probe cannot be polled, since the bus must be held high to power it, so that
     * waits the full worst case.
     */
    DSTherm(ow).convertTempAll(parasite ? DSTherm::MAX_CONV_TIME : DSTherm::SCAN_BUS, parasite);
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
  float c = readC(addr);
  if (c == SYSTEM_ONEWIRE_DISCONNECTED_C) {
    converted = false; // Force requestIfDue() to convert again rather than re-read the same scratchpad
    requestIfDue();
    c = readC(addr);
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
