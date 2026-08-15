/* Frugal IoT - INA219 current, voltage and power monitor
 *
 * NOTE - AS OF 2026-08-10 THIS IS UNTESTED CODE
 *
 * TI INA219 high-side current/power monitor over I2C. Freestanding over System_I2C - no
 * external library - in the same spirit as Sensor_ms5803 and Sensor_BME280.
 *
 * CREDIT
 * ──────
 * No code is copied, but the register semantics and all of the arithmetic below were
 * cross-checked line by line against Rob Tillaart's implementation rather than written from
 * memory, and it is the reason we can be confident in them:
 *
 *   https://github.com/RobTillaart/INA219  - Copyright (c) Rob Tillaart, MIT licence
 *   (also at https://registry.platformio.org/libraries/robtillaart/INA219)
 *
 * Specifically confirmed there: register addresses 0x00..0x05; shunt voltage is signed with a
 * 10uV LSB; bus voltage is (raw >> 3) with a 4mV LSB and bit 0 as the math-overflow flag;
 * power LSB is 20 x the current LSB; current_LSB = maxCurrent / 32768; and the calibration
 * register is 0.04096 / (current_LSB * shunt_ohms).
 *
 * Rob's library is the better choice for most projects - MIT, no dependencies, actively
 * maintained. We rolled our own here only to avoid the dependency and to keep the I2C access
 * going through System_I2C.
 *
 * Outputs published to MQTT:
 *   <id>/shunt   - voltage across the shunt resistor, mV (signed, +/-320mV at gain /8)
 *   <id>/bus     - bus voltage at the load side of the shunt, V
 *   <id>/current - current through the shunt, mA (signed - negative means reverse flow)
 *   <id>/power   - power, mW (computed in the chip, not by us)
 *   <id>/load    - voltage at the supply side, V, = bus + shunt/1000
 *
 * CALIBRATION - READ THIS
 * ───────────────────────
 * current and power are meaningless until SENSOR_INA219_SHUNT_OHMS matches the resistor
 * actually fitted to your board. The failure is silent: shunt and bus stay perfectly correct
 * while current and power are wrong by the ratio of the two resistances. The common Adafruit
 * and generic-purple breakouts fit 0.1 ohm, which is the default here; boards built for high
 * current often fit 0.002 ohm, which would read 50x low against this default.
 *
 * Build flags:
 *   SENSOR_INA219_ADDRESS     (0x40)   - 0x40..0x4F via the A0/A1 links
 *   SENSOR_INA219_SHUNT_OHMS  (0.1)    - MUST match the resistor on the board, see above
 *   SENSOR_INA219_MAX_CURRENT (3.2)    - largest current you expect, A. Sets the resolution:
 *                                        current LSB = MAX_CURRENT / 32768. At 0.1 ohm the
 *                                        hardware ceiling is 0.32V/0.1 = 3.2A at gain /8.
 *   SENSOR_INA219_CONFIG      (0x3FF8) - bus range, gain and ADC averaging bits. 32V range,
 *                                        gain /8, 128-sample averaging on both channels. The
 *                                        low 3 MODE bits are managed by this class and are
 *                                        masked off whatever you pass, so a datasheet-literal
 *                                        value like 0x3FFF works too. The chip's own reset
 *                                        default is 0x399F - same but single-sample; averaging
 *                                        costs ~68ms per channel and is much quieter.
 *   SENSOR_INA219_CONTINUOUS           - define to keep the ADC running all the time (the old
 *                                        behaviour). Costs ~1mA continuously. See Power below.
 *   SENSOR_INA219_DEBUG                - Serial debug output
 *
 * Power
 * ─────
 * Continuously converting costs about 1mA, which for a battery node is far more than the thing
 * it is measuring. So by default this class leaves the chip in MODE 000 (power-down, ~6uA) and
 * only wakes it for the moment it is being read:
 *
 *   read cycle : write MODE 011 (shunt+bus, triggered) -> poll the bus register's
 *                conversion-ready bit -> read the four registers -> write MODE 000 again
 *   before sleep: powerDown() sets MODE 000 (already there, but explicit and also correct when
 *                SENSOR_INA219_CONTINUOUS is set), then drops the power pins if any are wired
 *   after sleep : powerUp() restores the pins and flags the chip for reconfiguration
 *
 * This helps in every power mode, not only sleep: with Power_Loop nothing ever calls prepare()
 * or recover() (System_Power::prepare() is guarded by `if (mode)`), so a continuously
 * converting chip would draw its ~1mA forever.
 *
 * The triggered read blocks for the conversion - about 136ms with the default 128-sample
 * averaging on both channels. Set SENSOR_INA219_CONFIG to fewer samples if that matters, or
 * define SENSOR_INA219_CONTINUOUS to trade the power back for an instant read.
 *
 * Reconfiguration after waking is deliberately deferred to the next read rather than done in
 * powerUp(), because System_Power::recover() only does its SYSTEM_POWER_ON_DELAY *after* every
 * sensor's powerUp() has run - so I2C writes from inside powerUp() could hit a chip whose
 * supply is still ramping. Rewriting the calibration register as well as the config makes it
 * correct whether or not the board actually cut power to the chip.
 */

#ifndef SENSOR_INA219_H
#define SENSOR_INA219_H

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "system/i2c.h"
#include "sensor/sensor.h"

#ifndef SENSOR_INA219_ADDRESS
  #define SENSOR_INA219_ADDRESS 0x40
#endif
#ifndef SENSOR_INA219_SHUNT_OHMS
  #define SENSOR_INA219_SHUNT_OHMS 0.1
#endif
#ifndef SENSOR_INA219_MAX_CURRENT
  #define SENSOR_INA219_MAX_CURRENT 3.2
#endif
// Bus range / gain / ADC averaging bits. The low 3 MODE bits are managed by this class.
#ifndef SENSOR_INA219_CONFIG
  #define SENSOR_INA219_CONFIG 0x3FF8
#endif

class Sensor_INA219 : public Sensor {
  public:
    Sensor_INA219(const char* const id, const char * const name,
      uint8_t address = SENSOR_INA219_ADDRESS, TwoWire* wire = &I2C_WIRE,
      float shunt_ohms = SENSOR_INA219_SHUNT_OHMS,
      float max_current = SENSOR_INA219_MAX_CURRENT,
      const bool retain = true);
    OUTfloat* shunt;   // mV
    OUTfloat* bus;     // V
    OUTfloat* current; // mA
    OUTfloat* power;   // mW
    OUTfloat* load;    // V
  protected:
    System_I2C interface;
    float shunt_ohms;
    float max_current;
    float current_lsb = 0.0; // Amps per bit, derived from max_current in setup()
    bool present = false;    // Set once the device has ACKed and been configured
    bool needs_config = false; // Set by powerUp(), acted on at the next read - see .h notes
    void setup() override;
    void readValidateConvertSet() override;
    void captiveLines(AsyncResponseStream* response) override;
    void powerDown() override; // Puts the chip in MODE 000 before dropping any power pins
    void powerUp() override;   // Restores power pins, defers reconfiguration to the next read
    void writeConfig(uint8_t mode);  // SENSOR_INA219_CONFIG with these MODE bits
    void configure();                // Calibration + config; safe to repeat after a power cut
    bool awaitConversion();          // Poll the conversion-ready bit; false on timeout
};

#endif // SENSOR_INA219_H
