/* Frugal IoT - SHT3x / SHT4x temperature and humidity sensor
 *
 * See sensor/sht.h for the flags and for why both families are always built.
 *
 * See the guide for instructions to build one at 
 * https://github.com/mitra42/frugal-iot/wiki/Building-a-temperature---humidity-sensor/ 
 *
 * Freestanding - no external library, just System_I2C. It replaces a version that drove
 * RobTillaart's SHT85 and SHT4x libraries, chosen between at compile time, and owes
 * those libraries the register values and the CRC routine:
 *   https://github.com/RobTillaart/SHT85
 *   https://github.com/RobTillaart/SHT4x
 *
 * We've only built our own so we can use System_I2C for all I2Cs across Frugal-IoT, if you
 * are doing independent work, with SHT3x SHT85 or SHT4x we still recommend using Rob's libraries.
 * 
 * Two bugs from that version are worth not reintroducing, because both looked like hardware:
 *
 *   - The libraries' dataReady() is a timer, not a question - it never asks the chip. Its
 *     SHT3x estimate needs 16ms to have elapsed while a high repeatability conversion is
 *     specified at up to 15.5ms, so the first conversion after a reset regularly outran it
 *     and the first reading of every boot came back nan. See readValidateConvertSet().
 *
 *   - begin() only checks the address is in range and sends a soft reset, and a single byte
 *     write is acknowledged by BOTH families - so a node built for the wrong chip reported
 *     "begin ok" and then read nothing forever. See detect().
*/

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "sensor/sht.h"

// SHT3x - 16 bit commands
#define SHT3x_CMD_SOFTRESET     0x30A2
#define SHT3x_CMD_READSTATUS    0xF32D
#define SHT3x_CMD_MEASURE       0x2400 // Single shot, no clock stretching, high repeatability
#define SHT3x_MEASURE_MS        16     // Datasheet table 4: 15.5ms max for high repeatability
#define SHT3x_RESET_MS          2      // Datasheet table 4: 1.5ms max

// SHT4x - one byte commands
#define SHT4x_CMD_SOFTRESET     0x94
#define SHT4x_CMD_SERIALNUMBER  0x89
#define SHT4x_CMD_MEASURE       0xFD   // High precision
#define SHT4x_MEASURE_MS        9      // Datasheet table 5: 8.3ms max for high precision
#define SHT4x_RESET_MS          2      // Datasheet table 5: 1ms max

#define SHT_POWERUP_MS          10     // Both datasheets want ~1ms after VDD is valid
#define SHT_POLL_INTERVAL_MS    2      // Conversions are ~10-16ms; keeps the retry count small
#define SHT_CRC_POLY            0x31   // CRC-8, init 0xFF, no final xor - same for both families

// The only two addresses either family offers, in the order they are tried. 0x44 first: it is
// what both default to, and 0x45 needs a link moved (or is a D1 shield).
static const uint8_t SHT_ADDRESSES[] = { 0x44, 0x45 };
#define SHT_ADDRESS_COUNT (sizeof(SHT_ADDRESSES) / sizeof(SHT_ADDRESSES[0]))

Sensor_SHT::Sensor_SHT(const char * const name, uint8_t address, TwoWire *wire, bool retain)
    : Sensor("sht", name, retain),
    temperature(new OUTfloat("sht", "temperature", "Temperature", 0, 1, DEFAULT_sht_temperature_min, DEFAULT_sht_temperature_max, DEFAULT_sht_temperature_color, false)),
    humidity(new OUTfloat("sht", "humidity", "Humidity", 0, 1, DEFAULT_sht_humidity_min, DEFAULT_sht_humidity_max, DEFAULT_sht_humidity_color, false)),
    interface(address, wire) {
  outputs.push_back(temperature);
  outputs.push_back(humidity);
  temperature->unit = "C";
  humidity->unit = "%";
  // Bus setup deliberately NOT here - see setup(). Constructing a sensor happens before
  // powerPins() has said which pin feeds the bus and before anything has driven it, so beginning
  // the bus here brings the pull-ups up on a part whose VDD is still floating, on any board where
  // a GPIO supplies it.
}

// CRC-8, polynomial 0x31, initialised to 0xFF, no final xor. Page 14 of the SHT3x datasheet;
// the SHT4x uses the same.
uint8_t Sensor_SHT::crc8(const uint8_t* data, uint8_t len) {
  uint8_t crc = 0xFF;
  for (uint8_t j = len; j; --j) {
    crc ^= *data++;
    for (uint8_t i = 8; i; --i) {
      crc = (crc & 0x80) ? (crc << 1) ^ SHT_CRC_POLY : (crc << 1);
    }
  }
  return crc;
}

// SHT3x commands are two bytes, big endian. SHT4x commands are one, so those go straight to
// interface.send(uint8_t).
bool Sensor_SHT::send3x(uint16_t cmd) {
  uint8_t buf[2] = { (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF) };
  return interface.send(buf, 2);
}

// See the note in sht.h. Two words is the longest reply either family sends.
bool Sensor_SHT::readWords(uint16_t* out, uint8_t words) {
  uint8_t buf[6];
  bool ok = (words <= 2) && interface.read(buf, words * 3);
  if (ok) {
    for (uint8_t i = 0; i < words; i++) {
      const uint8_t* w = buf + (i * 3);
      if (w[2] == crc8(w, 2)) {
        out[i] = ((uint16_t)w[0] << 8) | w[1];
      } else {
        ok = false;
      }
    }
  }
  return ok;
}

/* Which family is on the bus? - see probe3x/probe4x below for the two questions asked.
 *
 * The two are the same size, the same shape, sit at the same 0x44 and both acknowledge their
 * address, so nothing short of talking to them tells them apart. Each family is asked for
 * something only it can answer, and the CRC on the reply is what makes the answer trustworthy:
 *
 *   SHT3x  16 bit command 0xF32D returns the 3 byte status register. 0xF3 is not a command an
 *          SHT4x knows, and unknown commands are not acknowledged, so a 4x NACKs the write,
 *          supplies nothing to the read, and is left with nothing half-sent to recover from.
 *
 *   SHT4x  one byte command 0x89 returns a 6 byte serial number. An SHT3x reads 0x89 as the
 *          FIRST HALF of a 16 bit command, waits for a second byte that never comes, and
 *          NACKs the read.
 *
 * A failed probe costs one core-level I2C error line in the log. SHT3x is tried first because
 * it is the commoner part and the historical default, so those boards boot clean; defining
 * SENSOR_SHT_SHT4x flips the order for boards known to carry a 4x.
 */
bool Sensor_SHT::probe3x() {
  uint16_t words[1];
  bool found = send3x(SHT3x_CMD_READSTATUS) && readWords(words, 1);
  #ifdef SENSOR_SHT_DEBUG
    if (found) { Serial.print(id); Serial.print(F(": SHT3x, status 0x")); Serial.println(words[0], HEX); }
  #endif
  return found;
}

bool Sensor_SHT::probe4x() {
  uint16_t words[2];
  interface.send((uint8_t)SHT4x_CMD_SERIALNUMBER);
  delay(1); // The part needs a moment before the serial number can be read back
  bool found = readWords(words, 2);
  #ifdef SENSOR_SHT_DEBUG
    if (found) {
      Serial.print(id); Serial.print(F(": SHT4x, serial 0x"));
      Serial.print(words[0], HEX); Serial.println(words[1], HEX);
    }
  #endif
  return found;
}

Sensor_SHT_Family Sensor_SHT::detect() {
  Sensor_SHT_Family found = SHT_FAMILY_NONE;
  #ifdef SENSOR_SHT_SHT4x
    if      (probe4x()) found = SHT_FAMILY_4x;
    else if (probe3x()) found = SHT_FAMILY_3x;
  #else
    if      (probe3x()) found = SHT_FAMILY_3x;
    else if (probe4x()) found = SHT_FAMILY_4x;
  #endif
  return found;
}

void Sensor_SHT::setup() {
  Sensor::setup(); // powerUp() then readConfigFromFS - both before we touch the bus
  // De-duplicated per bus, so several I2C sensors can each call it. Under SYSTEM_I2C_DEBUG it
  // also scans the bus once, which is the quickest way to separate a wiring fault from a
  // protocol one. Doing it here rather than in the constructor means the rails are up by the
  // time the bus comes up - it also powers the bus itself if nothing else has.
  interface.initialize();
  interface.wire->setClock(100000); // Can probably go faster - as fast as 850k on SHT40
  delay(SHT_POWERUP_MS);
  /* Find the sensor, then identify it - see the SENSOR_SHT_ADDRESS note in sht.h for why the
   * address is searched at all, and why only when nothing has named one.
   *
   * isPresent() is a zero length probe, which the ESP32 core logs at log_v rather than log_e,
   * so an address with nothing on it is silent. Only a failed FAMILY probe costs an error
   * line, and gating detect() on isPresent() means an empty address never reaches one.
   */
  const bool autoAddress = (interface.addr == SENSOR_SHT_ADDRESS_AUTO);
  if (autoAddress) {
    for (uint8_t i = 0; (i < SHT_ADDRESS_COUNT) && (family == SHT_FAMILY_NONE); i++) {
      interface.addr = SHT_ADDRESSES[i];
      if (interface.isPresent()) {
        family = detect(); // Something is there; it may still not be an SHT, so keep looking
      }
    }
    if (family == SHT_FAMILY_NONE) {
      interface.addr = SHT_ADDRESSES[0]; // Leave it somewhere sensible for the status page
    }
  } else if (interface.isPresent()) {
    family = detect();
  }
  if (family == SHT_FAMILY_NONE) {
    Serial.print(id); Serial.print(F(": no SHT3x or SHT4x found at "));
    if (autoAddress) {
      Serial.print(F("0x44 or 0x45"));
    } else {
      Serial.print(F("0x")); Serial.print(interface.addr, HEX);
    }
    Serial.println(F(" - check wiring, power and address"));
    setupFailed();
  } else {
    // Soft reset, so the part starts from a known state whatever the last boot left behind
    if (family == SHT_FAMILY_3x) {
      send3x(SHT3x_CMD_SOFTRESET);
      delay(SHT3x_RESET_MS);
    } else {
      interface.send((uint8_t)SHT4x_CMD_SOFTRESET);
      delay(SHT4x_RESET_MS);
    }
    connected = true;
    #ifdef SENSOR_SHT_DEBUG
      Serial.print(id); Serial.print(F(": "));
      Serial.print(family == SHT_FAMILY_4x ? F("SHT4x") : F("SHT3x"));
      Serial.print(F(" at 0x")); Serial.println(interface.addr, HEX);
    #endif
  }
}

bool Sensor_SHT::validate(float temp, float humy) {
    // Reject if both temperature and humidity are zero simultaneously
    return !(temp == 0.0f && humy == 0.0f);
}

void Sensor_SHT::readValidateConvertSet() {
  if (family == SHT_FAMILY_NONE) {
    // Absent at setup() and not retried - say so rather than publishing nothing at all
    setOutputsInvalid();
  } else {
    uint16_t words[2];
    uint8_t settle;
    if (family == SHT_FAMILY_3x) {
      send3x(SHT3x_CMD_MEASURE);
      settle = SHT3x_MEASURE_MS;
    } else {
      interface.send((uint8_t)SHT4x_CMD_MEASURE);
      settle = SHT4x_MEASURE_MS;
    }
    delay(settle);
    /* Then retry the read until the part answers.
     *
     * Both commands above ask for a measurement with clock stretching disabled, and that is
     * how these parts report "still converting" - they NACK the read. So the read IS the
     * readiness test, and `settle` is only an estimate of when it is worth starting to ask.
     * The first conversion after a reset runs past the datasheet maximum on some parts, which
     * is exactly what used to drop the first reading of every boot.
     *
     * A NACKed attempt costs one core-level I2C error line in the log. Normally there are none.
     */
    uint32_t start = millis(); // Not sleepSafeMillis as this is a sub-second wait
    uint8_t attempts = 1;
    bool got = readWords(words, 2);
    while (!got && ((millis() - start) < SENSOR_SHT_TIMEOUT_MS)) {
      delay(SHT_POLL_INTERVAL_MS);
      attempts++;
      got = readWords(words, 2);
    }
    if (got) {
      // Temperature is the same formula on both families; humidity is not.
      float temp = -45.0f + (175.0f * words[0] / 65535.0f);
      float humy;
      if (family == SHT_FAMILY_3x) {
        humy = 100.0f * words[1] / 65535.0f;
      } else {
        humy = -6.0f + (125.0f * words[1] / 65535.0f);
        // The SHT4x formula can land just outside the physical range at the extremes
        if (humy < 0.0f) humy = 0.0f;
        else if (humy > 100.0f) humy = 100.0f;
      }
      // Note, not smoothing the data as it seems fairly stable and is float rather than bits anyway
      #ifdef SENSOR_SHT_DEBUG
        Serial.print(id); Serial.print(F(" "));
        Serial.print(temp, 1); Serial.print(F("°C\t"));
        Serial.print(humy, 1); Serial.print(F("%"));
        if (attempts > 1) { // Worth seeing - it says how far past the estimate this part runs
          Serial.print(F("  (ready after ")); Serial.print(millis() - start);
          Serial.print(F("ms, ")); Serial.print(attempts); Serial.print(F(" reads)"));
        }
        Serial.println();
      #endif
      // Only set values if they pass validation
      if (validate(temp, humy)) {
        connected = true;
        temperature->set(temp);
        humidity->set(humy);
      } else {
        setOutputsInvalid(); // Publish "no reading" rather than leaving the last one standing
        #ifdef SENSOR_SHT_DEBUG
          Serial.print(id); Serial.println(F(": reading failed validation"));
        #endif // SENSOR_SHT_DEBUG
      }
    } else {
      connected = false;
      setOutputsInvalid();
      #ifdef SENSOR_SHT_DEBUG
        Serial.print(id); Serial.print(F(": no data after ")); Serial.print(millis() - start);
        Serial.print(F("ms and ")); Serial.print(attempts); Serial.println(F(" reads"));
      #endif // SENSOR_SHT_DEBUG
    }
  }
}
// Deep Sleep issues: none - read fresh on each wake.
