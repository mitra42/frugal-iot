/* Frugal IoT - Modbus RTU over RS485
 *
 * NOTE - AS OF 2026-08-03 THIS IS UNTESTED CODE
 *
 * See system/modbus.h for the bus/device split, wiring, build flags and the retry strategy.
 */

#include "system/modbus.h" // Includes _settings.h, and is a no-op unless SYSTEM_MODBUS_WANT

#ifdef SYSTEM_MODBUS_WANT

#include <Arduino.h>

System_RS485* System_RS485::transacting = nullptr;

System_RS485::System_RS485(HardwareSerial* serial,
  uint8_t rx_pin, uint8_t tx_pin, uint8_t de_pin, uint8_t re_pin, uint32_t baud)
  : serial(serial),
    rx_pin(rx_pin),
    tx_pin(tx_pin),
    de_pin(de_pin),
    re_pin(re_pin == 0xFF ? de_pin : re_pin), // 0xFF means the breakout ties RE to DE
    baud(baud)
  { }

// Drive the transceiver into transmit (on) or receive (off).
// DE is active HIGH, RE is active LOW, so both follow the same level - and when the
// breakout ties them together this just writes the one pin twice.
// de_pin 0xFF means the transceiver handles direction itself, so there is nothing to drive.
void System_RS485::txEnable(bool on) {
  if (de_pin != 0xFF) {
    digitalWrite(re_pin, on ? HIGH : LOW);
    digitalWrite(de_pin, on ? HIGH : LOW);
  }
}
void System_RS485::preTransmission() {
  if (transacting) {
    transacting->txEnable(true);
  }
}
void System_RS485::postTransmission() {
  if (transacting) {
    transacting->txEnable(false);
  }
}

// Called from the setup() of every device on this bus, so it has to be idempotent
void System_RS485::initialize() {
  if (!initialized) {
    initialized = true;
    if (de_pin != 0xFF) { // 0xFF - transceiver switches direction on its own, leave its pins alone
      pinMode(de_pin, OUTPUT);
      pinMode(re_pin, OUTPUT);
      txEnable(false); // Idle listening
    }
    #ifdef ESP8266
      // ESP8266 UART pins are fixed in hardware (no arbitrary rx/tx remap like ESP32);
      // rx_pin/tx_pin are ignored here - use HardwareSerial::swap() for the alternate pair.
      serial->begin(baud, SERIAL_8N1);
    #else
      serial->begin(baud, SERIAL_8N1, rx_pin, tx_pin);
    #endif
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
    #ifdef SYSTEM_MODBUS_DEBUG
      Serial.print(F("RS485 rx=")); Serial.print(rx_pin);
      Serial.print(F(" tx=")); Serial.print(tx_pin);
      Serial.print(F(" de=")); Serial.print(de_pin);
      Serial.print(F(" re=")); Serial.print(re_pin);
      Serial.print(F(" baud=")); Serial.println(baud);
    #endif
  }
}

// Blocks until the slave answers or ModbusMaster's 2 s timeout expires - see "Timing" in the .h
bool System_RS485::readHoldingRegisters(uint8_t slave_id, uint16_t reg, uint16_t count) {
  node.begin(slave_id, *serial); // Re-bind the slave; does not disturb the callbacks
  transacting = this;
  last_result = node.readHoldingRegisters(reg, count);
  transacting = nullptr;
  #ifdef SYSTEM_MODBUS_DEBUG
    Serial.print(F("Modbus slave=")); Serial.print(slave_id);
    Serial.print(F(" reg=0x")); Serial.print(reg, HEX);
    Serial.print(F(" result=0x")); Serial.println(last_result, HEX);
  #endif
  return last_result == ModbusMaster::ku8MBSuccess;
}

uint16_t System_RS485::responseBuffer(uint8_t i) {
  return node.getResponseBuffer(i);
}

System_Modbus::System_Modbus(uint8_t slave_id, System_RS485* bus)
  : slave_id(slave_id), bus(bus) { }

void System_Modbus::initialize() {
  bus->initialize();
}

// A silent slave costs 2 s per attempt, so once one has failed we skip
// SYSTEM_MODBUS_RETRY_CYCLES read cycles before trying it again.
bool System_Modbus::readRegister(uint16_t reg, uint16_t* value) {
  bool ok = false;
  if (connected || (retry_countdown == 0)) {
    ok = bus->readHoldingRegisters(slave_id, reg, 1);
    if (ok) {
      *value = bus->responseBuffer(0);
    } else {
      retry_countdown = SYSTEM_MODBUS_RETRY_CYCLES;
      #ifdef SYSTEM_MODBUS_DEBUG
        Serial.print(F("Modbus slave=")); Serial.print(slave_id);
        Serial.print(F(" silent, skipping ")); Serial.print(SYSTEM_MODBUS_RETRY_CYCLES);
        Serial.println(F(" cycles"));
      #endif
    }
    connected = ok;
  } else {
    retry_countdown--;
  }
  return ok;
}

#endif // SYSTEM_MODBUS_WANT
