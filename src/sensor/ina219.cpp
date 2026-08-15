/* Frugal IoT - INA219 current, voltage and power monitor
 *
 * NOTE - AS OF 2026-08-10 THIS IS UNTESTED CODE
 *
 * See sensor/ina219.h for the calibration warning and the build flags.
 *
 * CREDIT - the register semantics, LSB scalings and calibration arithmetic in this file were
 * cross-checked against Rob Tillaart's implementation rather than written from memory:
 *   https://github.com/RobTillaart/INA219  - Copyright (c) Rob Tillaart, MIT licence
 */

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "sensor/ina219.h"

// All INA219 registers are 16 bit, big-endian
#define INA219_REG_CONFIG        0x00
#define INA219_REG_SHUNT_VOLTAGE 0x01
#define INA219_REG_BUS_VOLTAGE   0x02
#define INA219_REG_POWER         0x03
#define INA219_REG_CURRENT       0x04
#define INA219_REG_CALIBRATION   0x05

#define INA219_BUS_OVF           0x0001 // Bus register bit 0 - power/current math overflowed
#define INA219_BUS_CNVR          0x0002 // Bus register bit 1 - conversion ready
#define INA219_SHUNT_LSB_MV      0.01   // 10uV per bit, expressed in mV
#define INA219_BUS_LSB_V         0.004  // 4mV per bit, after shifting out the flag bits
#define INA219_POWER_LSB_FACTOR  20     // Power LSB is 20 x the current LSB
#define INA219_CAL_CONSTANT      0.04096

// Config register MODE bits 2:0
#define INA219_MODE_MASK         0x0007
#define INA219_MODE_POWERDOWN    0x0000 // ~6uA - ADC and most of the chip off
#define INA219_MODE_TRIGGERED    0x0003 // Shunt and bus, one conversion then idle
#define INA219_MODE_CONTINUOUS   0x0007 // Shunt and bus, converting forever (~1mA)

// The mode the chip sits in between reads
#ifdef SENSOR_INA219_CONTINUOUS
  #define INA219_MODE_IDLE       INA219_MODE_CONTINUOUS
#else
  #define INA219_MODE_IDLE       INA219_MODE_POWERDOWN
#endif

// 128-sample averaging is ~68.1ms per channel, so ~136ms for shunt and bus. Allow margin.
#define INA219_CONVERSION_TIMEOUT_MS 300

Sensor_INA219::Sensor_INA219(const char* const id, const char * const name,
  uint8_t address, TwoWire* wire, float shunt_ohms, float max_current, const bool retain)
  : Sensor(id, name, retain),
    interface(address, wire),
    shunt_ohms(shunt_ohms),
    max_current(max_current)
{
  //TODO-213 define min/max/color in the UX defaults (generate-defaults.js) - literals for now
  // Current and power ranges scale with max_current, which is why they are not plain literals
  const float max_ma = max_current * 1000.0;
  outputs.push_back(shunt   = new OUTfloat(id, "shunt",   "Shunt Voltage", 0, 2, -320, 320, "blue",   false));
  outputs.push_back(bus     = new OUTfloat(id, "bus",     "Bus Voltage",   0, 3, 0, 32,     "green",  false));
  outputs.push_back(current = new OUTfloat(id, "current", "Current",       0, 2, -max_ma, max_ma, "red", false));
  outputs.push_back(power   = new OUTfloat(id, "power",   "Power",         0, 2, 0, 32 * max_ma, "orange", false));
  outputs.push_back(load    = new OUTfloat(id, "load",    "Load Voltage",  0, 3, 0, 32,     "green",  false));
}

// SENSOR_INA219_CONFIG with the MODE bits replaced - the caller owns the mode, and whatever
// MODE bits were in the build flag are masked off so a datasheet-literal value still works.
void Sensor_INA219::writeConfig(uint8_t mode) {
  interface.sendRegister16(INA219_REG_CONFIG,
    (uint16_t)((SENSOR_INA219_CONFIG & ~INA219_MODE_MASK) | (mode & INA219_MODE_MASK)));
}

// Calibration then config. Repeatable - called again after a wake in case the board cut power
// to the chip, in which case its registers were lost.
void Sensor_INA219::configure() {
  // Resolution of the current register, and the calibration value that ties the chip's
  // internal multiplier to our shunt resistor. Without this current and power read zero.
  current_lsb = max_current / 32768.0;
  uint16_t calibration = (uint16_t)(INA219_CAL_CONSTANT / (current_lsb * shunt_ohms));
  interface.sendRegister16(INA219_REG_CALIBRATION, calibration);
  writeConfig(INA219_MODE_IDLE);
  #ifdef SENSOR_INA219_DEBUG
    Serial.print(F("INA219 at 0x")); Serial.print(interface.addr, HEX);
    Serial.print(F(" shunt=")); Serial.print(shunt_ohms, 4);
    Serial.print(F("ohm max_current=")); Serial.print(max_current, 3);
    Serial.print(F("A current_lsb=")); Serial.print(current_lsb, 8);
    Serial.print(F("A calibration=")); Serial.print(calibration);
    Serial.print(F(" idle_mode=")); Serial.println(INA219_MODE_IDLE);
  #endif
}

void Sensor_INA219::setup() {
  Sensor::setup(); // Will readConfigFromFS - do before touching the device
  interface.initialize();
  if (!interface.isPresent()) { // Cheap ACK test before anything chip-specific
    Serial.print(F("INA219: nothing responding at 0x")); Serial.print(interface.addr, HEX);
    Serial.println(F(" - check address links and wiring"));
    setupFailed();
  } else if ((shunt_ohms <= 0.0) || (max_current <= 0.0)) {
    // Would divide by zero below, and current/power would be nonsense anyway
    Serial.println(F("INA219: SENSOR_INA219_SHUNT_OHMS and _MAX_CURRENT must both be > 0"));
    setupFailed();
  } else {
    configure();
    present = true;
    connected = true;
  }
}

// Called via Sensor::prepare() just before sleeping. Put the chip to sleep *before* the base
// class drops any power pins, so the write still gets through.
void Sensor_INA219::powerDown() {
  if (present) {
    writeConfig(INA219_MODE_POWERDOWN);
  }
  Sensor::powerDown(); // System_SensorActuator - drops power3v3_/power0v_ if wired
}

// Called via Sensor::recover() after waking. The actual register writes are deferred to the
// next read because System_Power::recover() runs its SYSTEM_POWER_ON_DELAY only after every
// sensor's powerUp() has returned - see the note in the .h
void Sensor_INA219::powerUp() {
  Sensor::powerUp();
  needs_config = true;
}

// Poll the bus register's conversion-ready bit. Note reading the Power register clears it, so
// this has to happen before the data reads.
bool Sensor_INA219::awaitConversion() {
  const uint32_t start = millis(); // Not sleepSafeMillis - this is a sub-second wait
  bool ready = false;
  while (!ready && ((millis() - start) < INA219_CONVERSION_TIMEOUT_MS)) {
    if (interface.send1read(INA219_REG_BUS_VOLTAGE, 2) & INA219_BUS_CNVR) {
      ready = true;
    }
  }
  #ifdef SENSOR_INA219_DEBUG
    if (!ready) {
      Serial.println(F("INA219: conversion did not complete in time"));
    }
  #endif
  return ready;
}

void Sensor_INA219::readValidateConvertSet() {
  // A device that was absent at setup() is not retried - restart after fixing the wiring,
  // same behaviour as Sensor_BME280
  if (present) {
    if (needs_config) { // Woke from sleep - the chip may have lost its registers
      configure();
      needs_config = false;
    }
    bool ready = true;
    #ifndef SENSOR_INA219_CONTINUOUS
      // Wake the ADC for exactly one conversion rather than letting it run at ~1mA
      writeConfig(INA219_MODE_TRIGGERED);
      ready = awaitConversion();
    #endif
    if (!ready) {
      connected = false;
    } else {
      const int16_t  raw_shunt   = (int16_t)interface.send1read(INA219_REG_SHUNT_VOLTAGE, 2);
      const uint16_t raw_bus     = (uint16_t)interface.send1read(INA219_REG_BUS_VOLTAGE, 2);
      const uint16_t raw_power   = (uint16_t)interface.send1read(INA219_REG_POWER, 2);
      const int16_t  raw_current = (int16_t)interface.send1read(INA219_REG_CURRENT, 2);
      if (raw_bus == 0xFFFF) {
        // Every bit high means nothing drove the bus - the device has gone away
        connected = false;
        #ifdef SENSOR_INA219_DEBUG
          Serial.println(F("INA219: all 1s - device not responding"));
        #endif
      } else if (raw_bus & INA219_BUS_OVF) {
        // The chip's own power/current multiplication overflowed, so those two are invalid.
        // Shunt and bus are still good, so publish them and skip the derived pair.
        shunt->set(raw_shunt * INA219_SHUNT_LSB_MV);
        bus->set((raw_bus >> 3) * INA219_BUS_LSB_V);
        Serial.println(F("INA219: math overflow - raise SENSOR_INA219_MAX_CURRENT"));
      } else {
        const float shunt_mv   = raw_shunt * INA219_SHUNT_LSB_MV;
        const float bus_v      = (raw_bus >> 3) * INA219_BUS_LSB_V; // Low 3 bits are flags
        const float current_ma = raw_current * current_lsb * 1000.0;
        const float power_mw   = raw_power * current_lsb * INA219_POWER_LSB_FACTOR * 1000.0;
        const float load_v     = bus_v + shunt_mv / 1000.0; // Supply side = load side + drop
        #ifdef SENSOR_INA219_DEBUG
          Serial.print(F("INA219 shunt=")); Serial.print(shunt_mv, 2);
          Serial.print(F("mV bus=")); Serial.print(bus_v, 3);
          Serial.print(F("V current=")); Serial.print(current_ma, 2);
          Serial.print(F("mA power=")); Serial.print(power_mw, 2);
          Serial.print(F("mW load=")); Serial.print(load_v, 3); Serial.println(F("V"));
        #endif
        connected = true;
        shunt->set(shunt_mv);
        bus->set(bus_v);
        current->set(current_ma);
        power->set(power_mw);
        load->set(load_v);
      }
    }
    #ifndef SENSOR_INA219_CONTINUOUS
      // Back to ~6uA. The datasheet says a triggered conversion stops on its own, but being
      // explicit costs one I2C write and leaves the chip in a known state.
      writeConfig(INA219_MODE_POWERDOWN);
    #endif
  }
}

void Sensor_INA219::captiveLines(AsyncResponseStream* response) {
  response->print(String(F("<p><label>")) + name
    + "<br>Shunt: "   + shunt->StringValue()   + " mV"
    + "<br>Bus: "     + bus->StringValue()     + " V"
    + "<br>Current: " + current->StringValue() + " mA"
    + "<br>Power: "   + power->StringValue()   + " mW"
    + "<br>Load: "    + load->StringValue()    + " V</label></p>");
}
