/* Frugal IoT - Modbus RTU over RS485
 * 
 * NOTE - AS OF 2026-08-03 THIS IS UNTESTED CODE
 *
 * Two classes, split the same way as System_I2C is split from the TwoWire bus it is given:
 *
 *   System_RS485  - the physical connection: one UART, its rx/tx pins, and the half-duplex
 *                   transceiver's direction pins. Owns the single ModbusMaster instance.
 *                   One of these per transceiver; share it between every device on the bus.
 *   System_Modbus - one addressed slave on that bus. Holds the slave id, tracks whether the
 *                   device is answering, and is what a sensor keeps as a member - compare
 *                   Sensor_ms5803's `System_I2C interface;`.
 *
 * RS485 is multi-drop, so several System_Modbus (different slave ids) can share one
 * System_RS485. The slave id is re-bound before each transaction, which is cheap -
 * ModbusMaster::begin() only sets _u8MBSlave/_serial and leaves the callbacks alone.
 * Keeping one ModbusMaster per bus rather than per device also matters for RAM: each
 * instance carries two uint16_t[64] buffers, 256 bytes.
 *
 * Wiring:
 *   transceiver RO  -> SYSTEM_RS485_RX_PIN   (ESP receives)
 *   transceiver DI  -> SYSTEM_RS485_TX_PIN   (ESP transmits)
 *   transceiver DE  -> SYSTEM_RS485_DE_PIN   (driver enable,   active HIGH)
 *   transceiver RE  -> SYSTEM_RS485_RE_PIN   (receiver enable, active LOW)
 * Most breakouts tie DE and RE together - define DE only. Boards with automatic direction
 * control need neither: leave both undefined and no direction switching is done.
 *
 * Required library:
 *   4-20ma/ModbusMaster  (Doc Walker)
 *
 * Build flags:
 *   SYSTEM_MODBUS_WANT           - REQUIRED. Derived in _settings.h from any sensor that
 *                                  needs Modbus (e.g. SENSOR_ULTRASONIC_SLAVE_ID).
 *   SYSTEM_RS485_RX_PIN          - REQUIRED, and
 *   SYSTEM_RS485_TX_PIN          - REQUIRED. Constructor defaults for the UART pins.
 *   SYSTEM_RS485_DE_PIN  (0xFF)  - 0xFF = transceiver switches direction itself
 *   SYSTEM_RS485_RE_PIN  (0xFF)  - 0xFF = tied to DE
 *   SYSTEM_RS485_BAUD    (9600)
 *   SYSTEM_MODBUS_RETRY_CYCLES (10) - see "Timing" below
 *   SYSTEM_MODBUS_DEBUG          - Serial debug output
 *
 * Timing
 * ──────
 * ModbusMaster's response timeout is `static const uint16_t ku16MBResponseTimeout = 2000`
 * - compile-time, with no setter - so a slave that does not answer costs a full 2 s inside
 * loop(). That is fine for a device that is really there (the watchdog is 180 s), but it
 * would be paid on every read cycle for a device that is absent or miswired.
 *
 * So System_Modbus tracks `connected`. After a failed transaction it skips the next
 * SYSTEM_MODBUS_RETRY_CYCLES read attempts entirely rather than blocking on each, then
 * tries once more. A device that is powered up later, or reconnected, is therefore picked
 * up automatically, at a cost of one 2 s stall per SYSTEM_MODBUS_RETRY_CYCLES cycles
 * instead of one per cycle.
 */

#ifndef SYSTEM_MODBUS_H
#define SYSTEM_MODBUS_H

#include "_settings.h"

#ifdef SYSTEM_MODBUS_WANT

#include <Arduino.h>
#include <ModbusMaster.h>  // https://registry.platformio.org/libraries/4-20ma/ModbusMaster

#if !defined(SYSTEM_RS485_RX_PIN) || !defined(SYSTEM_RS485_TX_PIN)
  #error "Modbus is enabled but the RS485 UART pins are not set - define SYSTEM_RS485_RX_PIN and SYSTEM_RS485_TX_PIN in platformio.ini"
#endif

#ifndef SYSTEM_RS485_DE_PIN
  #define SYSTEM_RS485_DE_PIN 0xFF
#endif
#ifndef SYSTEM_RS485_RE_PIN
  #define SYSTEM_RS485_RE_PIN 0xFF
#endif
#ifndef SYSTEM_RS485_BAUD
  #define SYSTEM_RS485_BAUD 9600
#endif
#ifndef SYSTEM_MODBUS_RETRY_CYCLES
  #define SYSTEM_MODBUS_RETRY_CYCLES 10
#endif

// One physical RS485 connection - UART plus transceiver. Share between devices on the bus.
class System_RS485 {
  public:
    System_RS485(HardwareSerial* serial,
      uint8_t rx_pin = SYSTEM_RS485_RX_PIN, uint8_t tx_pin = SYSTEM_RS485_TX_PIN,
      uint8_t de_pin = SYSTEM_RS485_DE_PIN, uint8_t re_pin = SYSTEM_RS485_RE_PIN,
      uint32_t baud = SYSTEM_RS485_BAUD);
    void initialize();  // Idempotent - every device on the bus calls it from its own setup()
    // Blocking. Returns true on success, after which responseBuffer() holds the values.
    bool readHoldingRegisters(uint8_t slave_id, uint16_t reg, uint16_t count);
    uint16_t responseBuffer(uint8_t i);
    uint8_t lastResult() { return last_result; }
  protected:
    ModbusMaster node; // One per bus - re-bound to a slave id before each transaction
    HardwareSerial* serial;
    uint8_t rx_pin;
    uint8_t tx_pin;
    uint8_t de_pin;
    uint8_t re_pin;
    uint32_t baud;
    bool initialized = false;
    uint8_t last_result = 0;
    void txEnable(bool on);
    // ModbusMaster's callbacks are plain void(*)() with nowhere to pass an instance, so the
    // bus currently mid-transaction is parked in a static that the two trampolines read.
    // Safe with more than one bus because readHoldingRegisters() sets it around each - and
    // only each - blocking transaction.
    static System_RS485* transacting;
    static void preTransmission();
    static void postTransmission();
};

// One addressed slave on a System_RS485. Held by value as a member of the sensor that uses it.
class System_Modbus {
  public:
    System_Modbus(uint8_t slave_id, System_RS485* bus);
    void initialize();  // Brings up the underlying bus
    // True while the slave is answering. False suppresses most transactions - see "Timing".
    bool connected = false;
    // Read one holding register into *value. False if the read failed or was skipped.
    bool readRegister(uint16_t reg, uint16_t* value);
  protected:
    uint8_t slave_id;
    System_RS485* bus;
    uint8_t retry_countdown = 0; // Cycles still to skip before retrying a silent slave
};

#endif // SYSTEM_MODBUS_WANT
#endif // SYSTEM_MODBUS_H
