// Deep Sleep issues: auto-binding re-runs on every wake by design (see resolveUnbound); bindings made by hand are persisted and survive. A bus on a switched rail is re-scanned after a light sleep too, since powerDown() drops its enumeration - see powerDown().
/* Frugal-IoT - shared OneWire bus for DS18B20 (and any future 1-Wire device)
 *
 * Split the same way System_RS485 is split from System_Modbus, and System_I2C from the TwoWire
 * bus it is handed: one object per physical bus, shared by every device on it.
 *
 * Why this exists rather than each sensor owning its own OneWire:
 *
 * A convert is broadcast to every device on the bus and then BLOCKS until it completes - up to
 * 750ms at 12-bit resolution. A sensor owning its own bus object therefore pays that itself, so
 * three probes on one pin cost 2.25 seconds of blocking per read cycle for one conversion's
 * worth of information. Sharing turns that into one broadcast and N cheap scratchpad reads.
 *
 * Addressing is by ROM id, not by index. DallasTemperature's getTempCByIndex(), which this
 * replaced, re-walked the search tree on every read and returned whatever sat at that position,
 * so adding, removing or replacing a probe silently renumbered the others - two believable
 * temperatures attributed to the wrong things, with nothing reporting an error.
 *
 * Built on OneWireNg (pstolarz/OneWireNg) and its DSTherm driver, not Paul Stoffregen's OneWire and
 * DallasTemperature. OneWire 2.3.8 does not compile for the ESP32-C6 - it assigns a uint32_t to
 * GPIO registers that IDF 5 declares as structs - and DallasTemperature depends on it. OneWireNg's
 * own OneWire compatibility class would have kept DallasTemperature, but only through an
 * unpublished fork of it (the published one still pulls in Stoffregen's OneWire, giving two
 * OneWire.h), and DallasTemperature was only ever eight calls in onewire.cpp.
 */
#ifndef SYSTEM_ONEWIRE_H
#define SYSTEM_ONEWIRE_H

#include <Arduino.h>
#include <vector>
#include "OneWireNg_CurrentPlatform.h"
#include "drivers/DSTherm.h"
#include "system/interface.h"

/* The pins that switch power to the 1-Wire bus - the 4.7k pull-up as well as the probes.
 *
 * Applied to every bus as it is created. A board with two 1-Wire pins on different rails should
 * call powerPins() on whichever is the exception - System_OneWire::forPin(n)->powerPins(...).
 */
#ifndef SYSTEM_ONEWIRE_POWER3v3_PIN
  #define SYSTEM_ONEWIRE_POWER3v3_PIN PIN_NONE
#endif
#ifndef SYSTEM_ONEWIRE_POWER0_PIN
  #define SYSTEM_ONEWIRE_POWER0_PIN PIN_NONE
#endif

#ifndef SYSTEM_ONEWIRE_RESOLUTION
  #define SYSTEM_ONEWIRE_RESOLUTION 12 // bits - 12 gives 0.0625C and a 750ms conversion
#endif
// Shortest gap between convert broadcasts. Anything above the 750ms conversion is enough to stop
// several sensors on one bus each triggering their own within a single periodically() pass,
// while staying below any realistic cycle time so every cycle still gets a fresh reading.
#ifndef SYSTEM_ONEWIRE_RECONVERT_MS
  #define SYSTEM_ONEWIRE_RECONVERT_MS 1000
#endif

// Thermometers remembered per bus. Anything beyond this is ignored, with a message under debug.
#ifndef SYSTEM_ONEWIRE_MAX_DEVICES
  #define SYSTEM_ONEWIRE_MAX_DEVICES 8
#endif
// What tempC() returns for a probe that did not answer. DallasTemperature's DEVICE_DISCONNECTED_C,
// kept at the same value because OSPIT uses -127 for the same thing.
#define SYSTEM_ONEWIRE_DISCONNECTED_C -127
// What a DS18B20 scratchpad holds from power-on until its first conversion completes. Exact in the
// sixteenths getTemp2() returns, so comparing for equality is safe.
#define SYSTEM_ONEWIRE_POWERON_C 85.0f

#define SYSTEM_ONEWIRE_ADDRLEN 8       // A 1-Wire ROM id is 8 bytes
#define SYSTEM_ONEWIRE_ADDRSTRLEN 16   // ... or 16 characters as lower case hex

/* Implemented by anything that binds to one device on the bus - in practice Sensor_DS18B20.
 *
 * An interface rather than the bus knowing about Sensor_DS18B20 directly, so that system/ does
 * not have to include sensor/. It exists so the bus can do the one piece of reasoning no single
 * sensor can: matching up what is unbound with what is unclaimed, across every sensor on the bus.
 */
class OneWireDevice {
  public:
    virtual ~OneWireDevice() = default;
    virtual bool owBound() const = 0;
    virtual const uint8_t* owAddress() const = 0;  // Only meaningful when owBound()
    virtual void owBindTo(const uint8_t* addr) = 0;
};

// What diagnose() finds on the data line - see classifyLine() in onewire.cpp
enum OneWireLine { LINE_UNKNOWN, LINE_PULLED_UP, LINE_FLOATING, LINE_HELD_LOW, LINE_UNSTABLE };

class System_OneWire : public System_Interface {
  public:
    // Returns the shared bus for this pin, creating it on first use. This is what lets a sketch
    // add two sensors on the same pin and have them share a bus without knowing buses exist.
    static System_OneWire* forPin(uint8_t pin);

    void add(OneWireDevice* device); // Called by each sensor, so the bus can resolve bindings
    /* Bind the last unbound sensor to the last unclaimed probe, when there is exactly one of each.
     *
     * That single rule covers every case worth automating:
     *  - one sensor, one probe, nothing configured - the ordinary node, no configuration at all
     *  - a probe replaced on a multi-probe bus: the others keep their stored bindings, so the new
     *    probe and the orphaned sensor are the only two left over and are matched up
     *  - binding a three-probe bus by hand: name two and the third follows
     *
     * Anything more ambiguous than one-to-one is left alone deliberately - guessing which probe
     * is the air one and which is the battery one is exactly the silent mis-attribution that
     * addressing by ROM id exists to prevent.
     */
    void resolveUnbound();
    bool isClaimed(const uint8_t* addr);

    System_OneWire(uint8_t pin);
    void initialize() override;         // begin() + scan; idempotent, call from each device's setup()
    /* Forget what is on the bus as well as releasing the pins.
     *
     * The probes lose their scratchpads and their resolution setting along with their power, and
     * a ROM id read before the power cycle says nothing about what is on the bus after it - so
     * the next initialize() has to walk the bus again rather than trust the old enumeration.
     */
    bool powerDown() override;
    void rescanIfEmpty();               // Re-walk the bus only while nothing has ever been found
    uint8_t count();                    // Devices found by the last scan()
    bool addressAt(uint8_t index, uint8_t* addr); // Enumerate, for listing choices in the UX
    bool isPresent(const uint8_t* addr);
    float tempC(const uint8_t* addr);   // Converts if due, then reads this device's scratchpad

    // "28238abb51210109" <-> 8 bytes. Returns false on anything not exactly 16 hex characters.
    static bool addressFromString(const String& s, uint8_t* addr);
    static String addressToString(const uint8_t* addr);

    /* Why a bus that should have a probe on it has nothing: prints one verdict line (rail dragged
     * down / no pull-up on the line / line held low / pull-up not on the switched rail / pin cannot
     * drive / no presence pulse / probe holds the line after a reset / wrong family / search
     * failing), plus the raw checks under SYSTEM_ONEWIRE_DEBUG. Differs with and without a switched
     * rail - see onewire.cpp.
     * Called by Sensor_DS18B20::diagnose(); sets `diagnosed`, which a scan that finds something clears.
     */
    void diagnose();
    bool diagnosed = false;             // diagnose() has run since the bus last had probes on it
    const uint8_t pin;
  private:
    void scan();
    void setResolution(const OneWireNg::Id& id);
    float readC(const uint8_t* addr);   // This device's scratchpad, or SYSTEM_ONEWIRE_DISCONNECTED_C
    void requestIfDue(bool fullWait = false); // fullWait: the fixed 750ms rather than polling for completion
    std::vector<OneWireDevice*> users; // Sensors on this bus, for resolveUnbound()
    OneWireNg_CurrentPlatform ow;      // The bus driver for whichever chip this is built for
    OneWireNg::Id ids[SYSTEM_ONEWIRE_MAX_DEVICES]; // What the last scan() found - see addressAt()
    bool parasite = false;              // Some probe has VDD unconnected - set by scan()
    bool converted = false;             // False after any restart, so the first read always converts
    unsigned long lastConvertMs = 0;    // millis() not sleepSafeMillis - sub-cycle timing, and
                                        // `converted` covers the deep sleep restart
    uint8_t devices = 0;
};

#endif // SYSTEM_ONEWIRE_H
