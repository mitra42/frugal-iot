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
 * Build flags:
 *   SENSOR_SOILMODBUS_WANT      - REQUIRED. Enables this class, and SYSTEM_MODBUS_WANT with it.
 *   SENSOR_SOILMODBUS_REGISTER  (0x0000) - first of the two registers
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

class Sensor_SoilModbus : public Sensor {
  public:
    Sensor_SoilModbus(const char* const id, const char * const name, uint8_t slave_id,
      System_RS485* bus, const bool retain, uint16_t reg = SENSOR_SOILMODBUS_REGISTER);
    // Its own outputs rather than a shared temperature+humidity base - see "There is no
    // Sensor_HT" in CLAUDE.md
    OUTfloat* humidity;
    OUTfloat* temperature;
  protected:
    System_Modbus modbus; // This probe as an addressed slave on the shared bus
    uint16_t reg;
    void setup() override;
    void readValidateConvertSet() override;
    bool validate(float humy, float temp);
};

#endif // SENSOR_SOILMODBUS_WANT
#endif // SENSOR_SOILMODBUS_H
