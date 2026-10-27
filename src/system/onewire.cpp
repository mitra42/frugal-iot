/* Frugal-IoT - shared OneWire bus - see onewire.h for why this is shared rather than per-sensor */

#include "_settings.h"
#include <Arduino.h>
#include <vector>
#include <string.h> // for memcmp/memcpy
#include "system/onewire.h"
#ifdef ESP32
  #include "driver/gpio.h" // gpio_get_level and pull modes, for diagnose()
#endif
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
  if (devices > 0) {
    diagnosed = false; // So losing them again is diagnosed again
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

#ifdef ESP32
/* What is on the line, found by reading it under the chip's own weak (~45k) pull-up and then its
 * weak pull-down. Anything external that is stronger - the 4.7k, a probe holding it, a short - wins
 * both times; a line with nothing on it follows them. Left with neither pull, as OneWireNg set it.
 *
 * This is the check that matters most, because a line with no pull-up behaves like a capacitor:
 * driven low by a reset, it stays low, and the sample that follows looks exactly like a presence
 * pulse. That fooled us into blaming a probe for "latching" the line, through two boards, three
 * probes and the old OneWire library, when the pull-up was simply not reaching the pin.
 *
 * gpio_get_level() rather than digitalRead(): the core returns 0 for a pin its peripheral manager
 * does not have as GPIO, and OneWireNg configures this one through the IDF.
 */
static OneWireLine classifyLine(uint8_t pin) {
  const gpio_num_t g = (gpio_num_t)pin;
  gpio_set_pull_mode(g, GPIO_PULLUP_ONLY);
  delay(2);
  const int up = gpio_get_level(g);
  gpio_set_pull_mode(g, GPIO_PULLDOWN_ONLY);
  delay(2);
  const int down = gpio_get_level(g);
  gpio_set_pull_mode(g, GPIO_FLOATING);
  OneWireLine c;
  if (up && down) {
    c = LINE_PULLED_UP;
  } else if (up && !down) {
    c = LINE_FLOATING;
  } else if (!up && !down) {
    c = LINE_HELD_LOW;
  } else {
    c = LINE_UNSTABLE;
  }
  return c;
}
#endif

/* Why a bus that should have a probe on it has nothing - see onewire.h. About 10ms.
 *
 * Two shapes, depending on whether the probe and pull-up are on a switched rail
 * (SENSOR_DS18B20_POWER3v3_PIN, which powerPins() hands to this bus) or on the board's 3V3:
 *  - switched: the rail can be checked (does it reach HIGH; is the pull-up really on it) and the
 *    probe power-cycled, which tells a probe that latched the line low from a short to GND.
 *  - 3V3: none of that is possible, so a line held low gets a verdict that says so.
 * Software cannot see past the pull-up, so beyond that it narrows the fault to one side of it.
 */
void System_OneWire::diagnose() {
  diagnosed = true;
  const bool switched = (power3v3_ != PIN_NONE);
  int railHigh = -1;     // -1: not tested
  int pullOnRail = -1;   // -1: not tested
  int drives = -1;
  OneWireLine before = LINE_UNKNOWN;    // As found - after whatever the scans did to it
  OneWireLine cycled = LINE_UNKNOWN;    // After power-cycling the probe, switched rail only
  OneWireLine afterReset = LINE_UNKNOWN;
  #ifdef ESP32
    const gpio_num_t g = (gpio_num_t)pin;
    before = classifyLine(pin);
    if (switched) {
      // Does the rail actually get there? The input buffer is off on a plain OUTPUT, so turn it on
      // to read the pad rather than the register. HIGH but reading low: a short, or a reversed probe.
      gpio_input_enable((gpio_num_t)power3v3_);
      railHigh = gpio_get_level((gpio_num_t)power3v3_);
      // Is the pull-up on this rail? With the rail LOW a 4.7k to it beats the internal pull-up; a
      // 4.7k to unswitched 3V3 does not care. 5ms off also power-cycles the probe.
      digitalWrite(power3v3_, LOW);
      gpio_set_pull_mode(g, GPIO_PULLUP_ONLY);
      delay(5);
      pullOnRail = !gpio_get_level(g);
      gpio_set_pull_mode(g, GPIO_FLOATING);
      digitalWrite(power3v3_, HIGH);
      delay(5);
      cycled = classifyLine(pin);
    }
    // Can the pin pull the line low? 10us, which a probe reads as nothing more than a short slot
    gpio_set_level(g, 0);
    gpio_set_direction(g, GPIO_MODE_INPUT_OUTPUT);
    delayMicroseconds(10);
    drives = !gpio_get_level(g);
    gpio_set_direction(g, GPIO_MODE_INPUT);
  #endif
  const bool presence = (ow.reset() == OneWireNg::EC_SUCCESS);
  #ifdef ESP32
    delay(1); // A presence pulse is over in 240us; anything still low after 1ms is not one
    afterReset = classifyLine(pin);
  #endif
  // The range-loop in scan() stops on a bus or CRC error exactly as on "no more devices", so search
  // once by hand to see which it was - and once unfiltered, which finds a device that answers but
  // is not a thermometer family.
  OneWireNg::ErrorCode filtered = OneWireNg::EC_NO_DEVS;
  OneWireNg::ErrorCode unfiltered = OneWireNg::EC_NO_DEVS;
  OneWireNg::Id id;
  if (presence) {
    ow.searchReset();
    filtered = ow.search(id);
    #if (CONFIG_MAX_SEARCH_FILTERS > 0)
      ow.searchFilterDelAll();
      ow.searchReset();
      unfiltered = ow.search(id);
      DSTherm(ow).filterSupportedSlaves(); // Put the filter back for the real scans
    #endif
  }
  #ifdef SYSTEM_ONEWIRE_DEBUG
    // Line: 0 unknown, 1 pulled up, 2 floating, 3 held low, 4 unstable.
    // Search: OneWireNg::ErrorCode - 0 found one, 1 no devices, 2 bus error, 3 CRC error.
    Serial.printf("  %s before=%d railHigh=%d pullOnRail=%d cycled=%d drives=%d presence=%d afterReset=%d search=%d unfiltered=%d\n",
      switched ? "switched" : "3V3", before, railHigh, pullOnRail, cycled, drives, presence, afterReset, (int)filtered, (int)unfiltered);
  #endif
  // One verdict, the first that applies - each later one assumes the earlier checks passed. On a
  // switched rail the line is judged after the power-cycle; as found, it may just be a latched probe.
  const OneWireLine line = switched ? cycled : before;
  Serial.print(F("OneWire pin ")); Serial.print(pin); Serial.print(F(": "));
  if (railHigh == 0) {
    Serial.print(F("GPIO")); Serial.print(power3v3_);
    Serial.println(F(" is set HIGH but reads LOW - something drags the rail down: a short, or a probe with VDD and GND swapped"));
  } else if (line == LINE_FLOATING) {
    Serial.print(F("no pull-up on the line - the 4.7k is not reaching GPIO")); Serial.print(pin);
    Serial.println(switched ? F(" or the rail") : F(" or 3V3"));
  } else if (line == LINE_HELD_LOW) {
    if (switched) {
      Serial.println(F("line stays LOW even with the probe power-cycled - data shorted to GND, or the pull-up is not on the rail"));
    } else {
      Serial.println(F("line is held LOW - data shorted to GND, the pull-up not on 3V3, or a stuck probe (unplug its power to check)"));
    }
  } else if (line == LINE_UNSTABLE) {
    Serial.println(F("line reads differently each time - a loose connection, or a very weak pull-up"));
  } else if (pullOnRail == 0) {
    Serial.print(F("the pull-up is not on GPIO")); Serial.print(power3v3_);
    Serial.println(F(" - it stays up with the rail off, so it is on unswitched 3V3 and will feed the probe while it sleeps"));
  } else if (drives == 0) {
    Serial.println(F("this pin cannot pull the line low - something else is driving it"));
  } else if (!presence) {
    Serial.println(F("pull-up is fine but no probe answers a reset - check probe GND, its data wire, and the probe itself"));
  } else if (afterReset == LINE_HELD_LOW) {
    Serial.println(F("a probe answered the reset then held the line LOW - a faulty or counterfeit probe"));
  } else if ((filtered != OneWireNg::EC_SUCCESS) && (unfiltered == OneWireNg::EC_SUCCESS)) {
    Serial.print(F("a device answers but is not a supported thermometer: "));
    Serial.println(addressToString(id));
  } else if (filtered != OneWireNg::EC_SUCCESS) {
    Serial.print(F("a probe answers but the search fails (code ")); Serial.print((int)filtered);
    Serial.println(F(") - signal quality: pull-up value, cable length"));
  } else {
    Serial.println(F("a probe answered this time - intermittent, check connections"));
  }
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
void System_OneWire::requestIfDue(bool fullWait) {
  const unsigned long now = millis();
  if (!converted || ((now - lastConvertMs) >= SYSTEM_ONEWIRE_RECONVERT_MS)) {
    /* Blocks up to 750ms at 12-bit - the cost this class exists to share. With every probe
     * powered from VDD, SCAN_BUS polls for completion and returns as soon as they are done; a
     * parasitic probe cannot be polled, since the bus must be held high to power it, so that
     * waits the full worst case. So does a retry - see tempC().
     */
    DSTherm(ow).convertTempAll((parasite || fullWait) ? DSTherm::MAX_CONV_TIME : DSTherm::SCAN_BUS, parasite);
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
 *
 * 85.00 is retried as well as the disconnected value. It is the scratchpad's power-on content, so
 * reading it means no conversion finished before the read - seen on the first hardware run
 * (supermini, 2026-10-27), where the first two reads after boot came back 85.00 and were published
 * as nan. validate() rejects anything from 80 up, so retrying a genuine 85C loses nothing. The retry
 * waits the fixed 750ms rather than polling the bus for completion, because a poll that returned
 * too early is the likeliest way to have read a scratchpad no conversion had filled yet.
 */
float System_OneWire::tempC(const uint8_t* addr) {
  requestIfDue();
  float c = readC(addr);
  if ((c == SYSTEM_ONEWIRE_DISCONNECTED_C) || (c == SYSTEM_ONEWIRE_POWERON_C)) {
    #ifdef SYSTEM_ONEWIRE_DEBUG
      const float first = c;
    #endif
    converted = false; // Force requestIfDue() to convert again rather than re-read the same scratchpad
    requestIfDue(true);
    c = readC(addr);
    #ifdef SYSTEM_ONEWIRE_DEBUG
      Serial.print(F("OneWire pin ")); Serial.print(pin);
      Serial.print(F(": ")); Serial.print(addressToString(addr));
      Serial.print(F(" read ")); Serial.print(first);
      Serial.print(F(", retried with a full 750ms conversion: ")); Serial.println(c);
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
