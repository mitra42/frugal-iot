// Deep Sleep issues: auto-binding re-runs on every wake by design (see resolveUnbound); bindings made by hand are persisted and survive.
/* Frugal-IoT - shared OneWire bus for DS18B20 (and any future 1-Wire device)
 *
 * Split the same way System_RS485 is split from System_Modbus, and System_I2C from the TwoWire
 * bus it is handed: one object per physical bus, shared by every device on it.
 *
 * Why this exists rather than each sensor owning its own OneWire:
 *
 * DallasTemperature::requestTemperatures() broadcasts a convert to every device on the bus and
 * then BLOCKS until it completes - 750ms at 12-bit resolution, because waitForConversion
 * defaults to true. A sensor owning its own bus object therefore pays that 750ms itself, so
 * three probes on one pin cost 2.25 seconds of blocking per read cycle for one conversion's
 * worth of information. Sharing turns that into one broadcast and N cheap scratchpad reads.
 *
 * Addressing is by ROM id, not by index. getTempCByIndex() re-walks the OneWire search tree on
 * every read and returns whatever sits at that position, so adding, removing or replacing a
 * probe silently renumbers the others - two believable temperatures attributed to the wrong
 * things, with nothing reporting an error.
 */
#ifndef SYSTEM_ONEWIRE_H
#define SYSTEM_ONEWIRE_H

#include <Arduino.h>
#include <vector>
#include <OneWire.h>
#include <DallasTemperature.h>

#ifndef SYSTEM_ONEWIRE_RESOLUTION
  #define SYSTEM_ONEWIRE_RESOLUTION 12 // bits - 12 gives 0.0625C and a 750ms conversion
#endif
// Shortest gap between convert broadcasts. Anything above the 750ms conversion is enough to stop
// several sensors on one bus each triggering their own within a single periodically() pass,
// while staying below any realistic cycle time so every cycle still gets a fresh reading.
#ifndef SYSTEM_ONEWIRE_RECONVERT_MS
  #define SYSTEM_ONEWIRE_RECONVERT_MS 1000
#endif

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

class System_OneWire {
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
    void initialize();                  // begin() + resolution; idempotent, call from each device's setup()
    uint8_t count();                    // Devices found at initialize()
    bool addressAt(uint8_t index, uint8_t* addr); // Enumerate, for listing choices in the UX
    bool isPresent(const uint8_t* addr);
    float tempC(const uint8_t* addr);   // Converts if due, then reads this device's scratchpad

    // "28238abb51210109" <-> 8 bytes. Returns false on anything not exactly 16 hex characters.
    static bool addressFromString(const String& s, uint8_t* addr);
    static String addressToString(const uint8_t* addr);

    const uint8_t pin;
  private:
    void requestIfDue();
    std::vector<OneWireDevice*> users; // Sensors on this bus, for resolveUnbound()
    OneWire wire;
    DallasTemperature dallas;
    bool initialized = false;
    bool converted = false;             // False after any restart, so the first read always converts
    unsigned long lastConvertMs = 0;    // millis() not sleepSafeMillis - sub-cycle timing, and
                                        // `converted` covers the deep sleep restart
    uint8_t devices = 0;
};

#endif // SYSTEM_ONEWIRE_H
