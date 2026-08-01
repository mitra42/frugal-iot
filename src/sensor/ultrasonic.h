/*
 * Ultrasonic distance sensor over RS485 / Modbus RTU (e.g. A01ANY4B).
 *
 * NOTE - AS OF 2026-08-03 THIS IS UNTESTED CODE
 *
 * Based on the Sensor Tambak water-quality node documented at
 * https://wiki-colabs.commonroom.info/Sensor_Tambak - note that the wiki's snippet is
 * incomplete (it never opens the UART, never calls node485.begin(), and never registers
 * the pre/postTransmission callbacks) so the transceiver setup is new.
 *
 * The module is a Modbus RTU slave; a single holding register holds the measured
 * distance from the sensor face to the reflecting surface, in millimetres.
 *
 * All the RS485/Modbus machinery lives in system/modbus.h - this class just says which
 * register to read and how to scale it. Wiring and bus flags (SYSTEM_RS485_*) are
 * documented there. Pass in the shared System_RS485; the System_Modbus for this slave is
 * built here as a member, the same way Sensor_ms5803 builds its System_I2C.
 *
 * Build flags:
 *   SENSOR_ULTRASONIC_SLAVE_ID   - REQUIRED. Defining it is what enables this sensor, and
 *                                  it also enables SYSTEM_MODBUS_WANT (see _settings.h).
 *                                  It is the module's Modbus slave address, usually 1.
 *   SENSOR_ULTRASONIC_REGISTER   (0x0101) - holding register holding the distance
 *   SENSOR_ULTRASONIC_DEBUG      - enable Serial debug output
 *
 * Output published to MQTT:
 *   <id>/<id>  - offset + raw * scale
 *
 * The raw register value is in millimetres, so the defaults (offset 0, scale 1) publish
 * the distance down to the surface in mm. To publish the depth of water in a tank of
 * known height instead - what the wiki's `200.0 - jarak / 10.0` was computing - set
 * scale to -1 and offset to the height of the sensor above the tank floor in mm.
 * Both are persisted to LittleFS and settable over MQTT; offset is also editable in the
 * captive portal (scale is not - the portal's number fields are step=1 so a fractional
 * scale could not be typed into one).
 */

#ifndef SENSOR_ULTRASONIC_H
#define SENSOR_ULTRASONIC_H

#include "_settings.h"  // Settings for what to include etc

// Enabled by defining the module's slave id, which also turns on SYSTEM_MODBUS_WANT
#ifdef SENSOR_ULTRASONIC_SLAVE_ID

#include "sensor/float.h"
#include "system/modbus.h"

#ifndef SENSOR_ULTRASONIC_REGISTER
  #define SENSOR_ULTRASONIC_REGISTER 0x0101
#endif

class Sensor_Ultrasonic : public Sensor_Float {
  public:
    Sensor_Ultrasonic(const char* const id, const char * const name, float max, const char* color, const bool retain,
      System_RS485* bus, float offset = 0.0, float scale = 1.0,
      uint8_t slave_id = SENSOR_ULTRASONIC_SLAVE_ID, uint16_t reg = SENSOR_ULTRASONIC_REGISTER);
  protected:
    System_Modbus modbus; // This module as an addressed slave on the shared bus
    float offset;
    float scale;
    uint16_t reg;
    void setup() override;
    float readFloat() override;
    float convert(float v) override;
    void dispatch(System_Message &msg) override;
    void captiveLines(AsyncResponseStream* response) override;
};
#endif // SENSOR_ULTRASONIC_SLAVE_ID
#endif // SENSOR_ULTRASONIC_H
