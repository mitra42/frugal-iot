/* Frugal-IoT DS18B20 sensor - see ds18b20.h for the binding rules

It should be noted, that it seems to require an ~4.7k resistor between data and positive.
Our tests are run at 3.3V though it is supposed to also run at 5V.
*/
#include "sensor/ds18b20.h"
#include "system/frugal.h"
#include "system/language.h" // for Texts
#include <cmath>
#include <string.h> // for memcpy

Sensor_DS18B20::Sensor_DS18B20(const char* id, const char* name, System_OneWire* bus, bool retain)
  : Sensor_Float(id, name, 1, DEFAULT_ds18b20_ds18b20_min, DEFAULT_ds18b20_ds18b20_max, DEFAULT_ds18b20_ds18b20_color, retain),  // width=1 for 1 decimal place
    bus(bus) {
  //TODO-213 fix this: setDefaultColor(DEFAULT_ds18b20_ds18b20_color);
}

Sensor_DS18B20::Sensor_DS18B20(const char* id, const char* name, uint8_t pin, bool retain)
  : Sensor_DS18B20(id, name, System_OneWire::forPin(pin), retain) { }

void Sensor_DS18B20::setup() {
    /* Sensor_Float::setup() FIRST, as in every other sensor, because it is what calls powerUp()
     * and what reads config from the filesystem - which may dispatch a stored id, so the binding
     * is known before the scan below.
     *
     * The bus's own rail is up by now whatever the order: powerPins() on a 1-Wire sensor goes to
     * the BUS (see system/interface.h), System_Frugal::setup() powers every bus before any
     * module's setup() runs, and bus->initialize() powers it again for anything reached outside
     * that lifecycle. Before all three of those existed, a scan placed ahead of this line
     * searched a bus held LOW by an undriven power pin and found nothing.
     */
    Sensor_Float::setup();
    bus->initialize();     // Idempotent - every probe on this bus calls it
    bus->add(this);        // So the bus can match unbound sensors to unclaimed probes
    if (bound && !bus->isPresent(addr)) {
        // Configured for a probe that is not on the bus. Drop the binding rather than read
        // whatever else is there - resolveUnbound() may well re-match this sensor to a
        // replacement probe. The stored id is left on disk on purpose: if that probe is ever
        // reconnected, the explicit choice should win again.
        bound = false;
        #ifdef SENSOR_DS18B20_DEBUG
            Serial.print(id); Serial.println(F(": bound probe not on the bus"));
        #endif
    }
    // Not resolved here: the other sensors on this bus have not necessarily run setup() yet, so
    // which of them are unbound is not yet known. Deferred to the first read - see readFloat().
}

bool Sensor_DS18B20::setAddress(const String& s) {
    const bool ok = System_OneWire::addressFromString(s, addr);
    if (ok) {
        bound = true;
        resolved = false; // Binding one sensor can leave exactly one other to be matched up
    }
    return ok;
}

void Sensor_DS18B20::owBindTo(const uint8_t* a) {
    memcpy(addr, a, SYSTEM_ONEWIRE_ADDRLEN);
    bound = true;
    // Deliberately not written to the filesystem. Storing an automatic match would mean that
    // replacing this probe left the node bound to an id that no longer exists - turning a setup
    // that works into one that does not, for no gain, since the same match is made again next boot.
    #ifdef SENSOR_DS18B20_DEBUG
        Serial.print(id); Serial.print(F(": auto-bound to ")); Serial.println(System_OneWire::addressToString(addr));
    #endif
}

bool Sensor_DS18B20::validate(float v) {
    // DEVICE_DISCONNECTED_C is -127, and 85C is the power-on reset value of an uninitialised
    // probe. 0.0C is NOT excluded - it is a real temperature, and excluding it made a probe at
    // freezing publish "no reading".
    return !std::isnan(v) && (v > DEVICE_DISCONNECTED_C) && (v < 80);
}

float Sensor_DS18B20::readFloat() {
    if (!resolved) {
        // First read after setup, or after a binding changed. Every sensor on this bus has run
        // setup() by now - periodically() only runs once the whole group is set up - so which
        // sensors are unbound is finally known.
        bus->resolveUnbound();
        /* Latched only once it worked. A bus that scanned empty is worth trying again on the
         * next read - a probe plugged in after boot, or one whose power came up late - whereas
         * latching on the first attempt turns a momentary empty scan into a sensor that never
         * reads again. Once bound there is nothing left to resolve, so this runs at most once
         * per read cycle and stops entirely as soon as it succeeds.
         */
        resolved = bound;
    }
    float tempC = NAN;
    if (!bound) {
        // Nothing to read from. Returning NAN publishes the invalid state, which is the honest
        // answer and is visible in the UX, rather than silently reporting some other probe.
        #ifdef SENSOR_DS18B20_DEBUG
            Serial.print(id); Serial.println(F(": unbound - set its id in the portal"));
        #endif
    } else {
        tempC = bus->tempC(addr);
        #ifdef SENSOR_DS18B20_DEBUG
            Serial.print(id); Serial.print(F(" returned:")); Serial.println(tempC);
        #endif
    }
    return tempC; // validate() turns the disconnected sentinel into "no reading"
}

void Sensor_DS18B20::dispatch(System_Message &msg) {
    if (msg.isSet() && (msg.module() == id) && (msg.leaf() == "id")) {
        if (setAddress(msg.payload)) {
            msg.maybeWriteToFSandEcho(); // Persisted: which probe is which survives a reboot
        } else {
            Serial.print(id); Serial.print(F(": not a 1-Wire id: ")); Serial.println(msg.payload);
        }
    } else {
        Sensor::dispatch(msg);
    }
}

/* The reading, plus - when there is a choice to make - the ids actually on the bus.
 *
 * Only shown when there is more than one probe, because with one there is nothing to choose and
 * a row of hex would be noise. Readable from a phone on the node's own AP, so binding a probe
 * never needs a serial cable.
 */
void Sensor_DS18B20::captiveLines(AsyncResponseStream* response) {
    Sensor_Float::captiveLines(response);
    const uint8_t n = bus->count();
    if (n > 1) {
        response->print(String(F("<p><label>")) + name + " " + T->OneWireProbe + ":<br>");
        response->print(String(F("<select name='")) + id + "/id' onchange=\"s(this.name,this.value)\">");
        uint8_t a[SYSTEM_ONEWIRE_ADDRLEN];
        for (uint8_t i = 0; i < n; i++) {
            if (bus->addressAt(i, a)) {
                const String s = System_OneWire::addressToString(a);
                const bool isMine = bound && (memcmp(a, addr, SYSTEM_ONEWIRE_ADDRLEN) == 0);
                response->print(String(F("<option value='")) + s + "'" + (isMine ? " selected" : "") + ">" + s + "</option>");
            }
        }
        if (!bound) { // Nothing chosen yet - do not let the first entry look like a choice made
            response->print(String(F("<option value='' selected>")) + T->OneWireUnbound + "</option>");
        }
        response->print(F("</select></label></p>"));
    }
}
