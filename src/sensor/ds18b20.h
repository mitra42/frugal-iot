// Deep Sleep issues: auto-binding re-runs each wake, which is cheap; an id chosen by hand is persisted.
/* DS18B20 waterproof temperature sensor
 *
 * Bound to a probe by its ROM id - its identity - not by its position in the bus enumeration.
 * Position is what the library's getTempCByIndex() uses, and it renumbers silently whenever a
 * probe is added, removed or replaced: two believable temperatures, attributed to the wrong
 * things, with nothing reporting an error.
 *
 * Binding is meant to need no attention in the ordinary case, and is never something to type
 * into a sketch:
 *
 *   - A stored binding whose probe is present: used.
 *   - Otherwise, if exactly one sensor on the bus is unbound and exactly one probe is unclaimed,
 *     they are matched up - deliberately NOT persisted, so replacing a probe keeps working. That
 *     one rule covers the ordinary single-probe node, a probe replaced on a multi-probe bus, and
 *     binding a three-probe bus by naming only two of them. See System_OneWire::resolveUnbound.
 *   - Anything more ambiguous: unbound, which publishes "nan" - see "Invalid readings" in
 *     CLAUDE.md - and the captive portal lists the ids actually on the bus so one can be chosen.
 *     Guessing between two unbound probes is the silent mis-attribution this exists to prevent.
 *
 * Binding is set/<sensorid>/id = <romid>, persisted to LittleFS, so the captive portal, the UX
 * and MQTT all reach it by the same path. Several probes on one bus are therefore given
 * meaningful ids in the sketch and bound once on site:
 *
 *   System_OneWire* ow = System_OneWire::forPin(SENSOR_DS18B20_PIN);
 *   frugal_iot.sensors->add(new Sensor_DS18B20("ds18b20-air",  "Air Temperature",     ow, true));
 *   frugal_iot.sensors->add(new Sensor_DS18B20("ds18b20-batt", "Battery Temperature", ow, true));
 */

#ifndef SENSOR_DS18B20_H
#define SENSOR_DS18B20_H

#include "sensor/float.h"
#include "system/onewire.h"

// Default power control pins - can be overridden via constructor parameters
#ifndef SENSOR_DS18B20_POWER0_PIN
  #define SENSOR_DS18B20_POWER0_PIN 0xff
#endif
#ifndef SENSOR_DS18B20_POWER3v3_PIN
  #define SENSOR_DS18B20_POWER3v3_PIN 0xff
#endif

class Sensor_DS18B20 : public Sensor_Float, public OneWireDevice {
public:
    /* pin form: the bus for that pin is looked up (and created once) by System_OneWire::forPin,
     * so a sketch adding two probes on the same pin gets a shared bus without knowing buses
     * exist - which matters, because the conversion that a shared bus does once costs 750ms.
     */
    Sensor_DS18B20(const char* id, const char* name, uint8_t pin, bool retain);
    // Explicit bus, matching how Sensor_Ultrasonic takes its System_RS485
    Sensor_DS18B20(const char* id, const char* name, System_OneWire* bus, bool retain);

protected:
    float readFloat() override;
    /* Rejects the disconnected sentinel and the 85C power-on value.
     * Note it does NOT reject 0.0C: that is a real temperature, and rejecting it made a probe at
     * freezing report "no reading" once invalid readings started being published.
     */
    bool validate(float v) override;
    void setup() override;
    void dispatch(System_Message &msg) override;
    void captiveLines(AsyncResponseStream* response) override;

    // OneWireDevice - lets the bus match unbound sensors to unclaimed probes across the whole bus,
    // which is the one decision no single sensor has the information to make for itself.
    bool owBound() const override { return bound; }
    const uint8_t* owAddress() const override { return addr; }
    void owBindTo(const uint8_t* a) override;

private:
    System_OneWire* bus;
    uint8_t addr[SYSTEM_ONEWIRE_ADDRLEN];
    bool bound = false;
    bool resolved = false;             // resolveUnbound has run since the last binding change
    bool setAddress(const String& s);  // From a stored or posted romid; false if unparsable
};

#endif // SENSOR_DS18B20_H
