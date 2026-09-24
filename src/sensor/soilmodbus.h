// Deep Sleep issues: address auto-provisioning state is lost, so it re-probes the factory address after each wake; ids already written into probes are in the probes and survive.
/* Soil moisture and temperature probe over RS485 / Modbus RTU (DFRobot SEN0600 and similar)
 *
 * NOTE - UNTESTED AGAINST HARDWARE
 *
 * The probe answers function 0x03 with two consecutive holding registers: moisture first, then
 * temperature, both scaled by ten. Temperature is signed two's complement; moisture is not. This
 * is what OSPIT's modbr.lua reads (`string.char(node, 0x03, 0, 0, 0, 2)`, then `h_raw/10` and a
 * sign-corrected `t_raw/10`), and this class is a direct port of that reading.
 *
 * One instance per probe, each with its own slave id, all sharing one System_RS485 - which is the
 * point of RS485 being multi-drop. That differs from Sensor_Ultrasonic, which is enabled by
 * SENSOR_ULTRASONIC_SLAVE_ID because a node has one of those; here a node has one per irrigation
 * sector, so the ids are constructor arguments and SENSOR_SOILMODBUS_WANT turns the class on.
 *
 *   System_RS485* rs485 = new System_RS485(&Serial2);
 *   frugal_iot.sensors->add(new Sensor_SoilModbus("soil1", "Sector 1", 1, rs485, true));
 *   frugal_iot.sensors->add(new Sensor_SoilModbus("soil2", "Sector 2", 2, rs485, true));
 *
 * A probe that does not answer publishes "nan" on both outputs rather than leaving stale readings
 * standing - see "Invalid readings" in CLAUDE.md. That is the same signal OSPIT carries as -127,
 * and it is what lets a control skip a sector whose probe is missing by asking isValid() rather
 * than comparing against a magic number. The bus's retry backoff means an absent probe costs one
 * 2s stall per SYSTEM_MODBUS_RETRY_CYCLES cycles, not one every cycle.
 *
 * ---------------------------------------------------------------------------------------------
 * Slave addresses, and why they cannot be discovered
 *
 * This is the opposite problem to the DS18B20's. A 1-Wire probe carries a factory-unique 64-bit
 * ROM id and the bus has a search primitive, so ids are DISCOVERED and can never collide. A
 * Modbus slave address is ASSIGNED, every probe ships with the same one (1), and there is no
 * enumeration and no readable unique id. Three fresh probes therefore all answer address 1, reply
 * on top of each other, and produce CRC failures rather than three readings.
 *
 * So the addresses have to be written, one probe at a time, and SENSOR_SOILMODBUS_AUTOPROVISION
 * does that automatically - see provision(). Note the consequence for the ids you choose:
 *
 *   DO NOT GIVE A PROBE THE FACTORY DEFAULT ADDRESS. Number them from 2 upward and leave 1
 *   meaning "not yet provisioned", or a provisioned probe is indistinguishable from a new one.
 *
 * Build flags:
 *   SENSOR_SOILMODBUS_WANT      - REQUIRED. Enables this class, and SYSTEM_MODBUS_WANT with it.
 *   SENSOR_SOILMODBUS_REGISTER  (0x0000) - first of the two registers
 *   SENSOR_SOILMODBUS_AUTOPROVISION - opt in to automatic address assignment (see provision())
 *   SENSOR_SOILMODBUS_FACTORY_ID (1) - the address an unprovisioned probe answers to
 *   SENSOR_SOILMODBUS_IDREGISTER (0x07D0) - holding register holding the probe's own address
 *   SENSOR_SOILMODBUS_DEBUG     - Serial debug output
 * Bus wiring and the SYSTEM_RS485_* flags are documented in system/modbus.h.
 *
 * Outputs published to MQTT:
 *   <id>/humidity     - volumetric water content, %
 *   <id>/temperature  - degrees C
 */

#ifndef SENSOR_SOILMODBUS_H
#define SENSOR_SOILMODBUS_H

#include "_settings.h"

#ifdef SENSOR_SOILMODBUS_WANT

#include "sensor/sensor.h"
#include "system/modbus.h"

#ifndef SENSOR_SOILMODBUS_REGISTER
  #define SENSOR_SOILMODBUS_REGISTER 0x0000
#endif
#ifndef SENSOR_SOILMODBUS_FACTORY_ID
  #define SENSOR_SOILMODBUS_FACTORY_ID 1
#endif
#ifndef SENSOR_SOILMODBUS_IDREGISTER
  // Where this class of probe keeps its own slave address. Same register OSPIT writes in
  // modb-set-nodeid.lua ("local TARGET_REG = 0x07D0", function code 0x06).
  #define SENSOR_SOILMODBUS_IDREGISTER 0x07D0
#endif

#include <vector>

class Sensor_SoilModbus : public Sensor {
  public:
    Sensor_SoilModbus(const char* const id, const char * const name, uint8_t slave_id,
      System_RS485* bus, const bool retain, uint16_t reg = SENSOR_SOILMODBUS_REGISTER);
    // Its own outputs rather than a shared temperature+humidity base - see "There is no
    // Sensor_HT" in CLAUDE.md
    OUTfloat* humidity;
    OUTfloat* temperature;
    /* Give the probe currently answering at the factory address THIS sensor's slave id.
     *
     * Returns false if nothing answered there, or the write failed. Safe to call at any time -
     * the transaction is addressed to the factory id, so a probe that already has its own address
     * is not touched.
     *
     * "Exactly one probe at the factory address" needs no counting: two probes sharing an address
     * answer simultaneously and wreck the frame, so a read that SUCCEEDS there is itself the proof
     * that only one is listening. That is why this starts with a read.
     */
    bool provision();
  protected:
    System_Modbus modbus; // This probe as an addressed slave on the shared bus
    // powerPins() applies to the shared RS485 bus, not to this slave - see system/interface.h
    System_Interface* powerInterface() override { return modbus.bus(); }
    uint8_t slave_id;     // Kept here too - provision() needs it, and it is what gets written
    uint16_t reg;
    /* Every probe constructed, in construction order - which is the order sectors were added, and
     * so the order a person plugs probes in. Only used by autoProvision().
     */
    static std::vector<Sensor_SoilModbus*> all;
    static uint8_t provision_countdown; // Rate-limits the probe of the factory address
    /* Assign an address to at most one probe per call, to the FIRST sensor that has no probe.
     *
     * So the field procedure is "plug the probes in in order": connect sector 2's probe, wait,
     * connect sector 3's, wait. Replacing a failed probe works without thinking about it, because
     * a bus with one dead sector has exactly one sensor waiting and the new probe is the only one
     * at the factory address.
     *
     * Only compiled in under SENSOR_SOILMODBUS_AUTOPROVISION. It writes to the user's hardware,
     * and a node whose addresses were set deliberately should not have them changed behind its
     * back, so it is opt-in rather than opt-out.
     */
    static void autoProvision();
    void setup() override;
    void readValidateConvertSet() override;
    bool validate(float humy, float temp);
    void dispatch(System_Message &msg) override;
    void captiveLines(AsyncResponseStream* response) override;
};

#endif // SENSOR_SOILMODBUS_WANT
#endif // SENSOR_SOILMODBUS_H
