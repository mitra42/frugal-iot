/* Soil moisture and temperature probe over RS485 / Modbus RTU - see soilmodbus.h */

#include "sensor/soilmodbus.h" // Includes _settings.h, and is a no-op unless the WANT flag is set

#ifdef SENSOR_SOILMODBUS_WANT

#include <Arduino.h>
#include "Frugal-IoT.h" // For frugal_iot

#define SENSOR_SOILMODBUS_COUNT 2 // Moisture then temperature, consecutive

Sensor_SoilModbus::Sensor_SoilModbus(const char* const id, const char * const name, uint8_t slave_id,
  System_RS485* bus, const bool retain, uint16_t reg)
  : Sensor(id, name, retain),
    humidity(new OUTfloat(id, "humidity", "Soil Moisture", 0, 1,
      DEFAULT_soilmodbus_humidity_min, DEFAULT_soilmodbus_humidity_max,
      DEFAULT_soilmodbus_humidity_color, false)),
    temperature(new OUTfloat(id, "temperature", "Soil Temperature", 0, 1,
      DEFAULT_soilmodbus_temperature_min, DEFAULT_soilmodbus_temperature_max,
      DEFAULT_soilmodbus_temperature_color, false)),
    modbus(slave_id, bus),
    slave_id(slave_id),
    reg(reg)
  {
    humidity->unit = "%";
    temperature->unit = "C";
    outputs.push_back(humidity);
    outputs.push_back(temperature);
    all.push_back(this); // Construction order is the order probes get provisioned - see the header
  }

std::vector<Sensor_SoilModbus*> Sensor_SoilModbus::all;
uint8_t Sensor_SoilModbus::provision_countdown = 0;

void Sensor_SoilModbus::setup() {
  Sensor::setup();
  modbus.initialize(); // Idempotent - every probe on this bus calls it
}

/* Plausibility only - the probe reports no status of its own, so a failed transaction is the
 * main signal and this catches a reply that arrived but cannot be real. Ranges come from the
 * schema rather than being invented here.
 */
bool Sensor_SoilModbus::validate(float humy, float temp) {
  return (humy >= 0.0f) && (humy <= 100.0f)
      && (temp >= -40.0f) && (temp <= 85.0f); // Datasheet operating range for this class of probe
}

bool Sensor_SoilModbus::provision() {
  bool done = false;
  if (slave_id == SENSOR_SOILMODBUS_FACTORY_ID) {
    // Nothing to do, and doing it would be indistinguishable from doing nothing - see the header
    Serial.print(id); Serial.println(F(": slave id is the factory default, cannot provision"));
  } else {
    // The read is the test for "exactly one probe is listening there" - two would collide
    uint16_t probe[SENSOR_SOILMODBUS_COUNT];
    if (!modbus.bus()->readHoldingRegisters(SENSOR_SOILMODBUS_FACTORY_ID, reg, SENSOR_SOILMODBUS_COUNT)) {
      #ifdef SENSOR_SOILMODBUS_DEBUG
        Serial.print(id); Serial.println(F(": nothing answering at the factory address"));
      #endif
    } else {
      (void)probe;
      done = modbus.bus()->writeSingleRegister(SENSOR_SOILMODBUS_FACTORY_ID,
                                               SENSOR_SOILMODBUS_IDREGISTER, slave_id);
      Serial.print(id);
      if (done) {
        Serial.print(F(": provisioned a probe as slave ")); Serial.println(slave_id);
      } else {
        Serial.println(F(": a probe answered at the factory address but would not take a new id"));
      }
    }
  }
  return done;
}

#ifdef SENSOR_SOILMODBUS_AUTOPROVISION
void Sensor_SoilModbus::autoProvision() {
  if (provision_countdown) {
    // An unanswered read at the factory address costs a full ModbusMaster timeout, so do not pay
    // it every cycle just because a sector is empty - same reasoning as System_Modbus's backoff.
    provision_countdown--;
  } else {
    provision_countdown = SYSTEM_MODBUS_RETRY_CYCLES;
    for (Sensor_SoilModbus* s : all) {
      if (!s->modbus.connected) {
        s->provision(); // At most one per call - the first sector still waiting for a probe
        break;
      }
    }
  }
}
#endif // SENSOR_SOILMODBUS_AUTOPROVISION

/* The one button: hand the probe at the factory address this sector's id.
 *
 * The fallback for a bus that already has several probes on it at the factory default, where
 * autoProvision() correctly refuses to guess - unplug all but one and press.
 */
void Sensor_SoilModbus::captiveLines(AsyncResponseStream* response) {
  Sensor::captiveLines(response);
  response->print(String(F("<p>")) + name + F(": slave ") + slave_id
    + (modbus.connected ? F(" (answering)") : F(" (no reply)")) + F("</p>"));
  frugal_iot.captive->addButton(response, id, "provision", "1", "Assign to new probe");
}

void Sensor_SoilModbus::dispatch(System_Message &msg) {
  if (msg.isSet() && (msg.module() == id) && (msg.leaf() == "provision")) {
    provision(); // Deliberately not persisted - it is an action, not a setting
  } else {
    Sensor::dispatch(msg);
  }
}

void Sensor_SoilModbus::readValidateConvertSet() {
  uint16_t raw[SENSOR_SOILMODBUS_COUNT] = {0, 0};
  if (!modbus.readRegisters(reg, SENSOR_SOILMODBUS_COUNT, raw)) {
    // No answer. Say so rather than leaving the previous reading standing - a control deciding
    // whether to open a valve needs to know the difference between "still 40%" and "no idea".
    setOutputsInvalid();
    #ifdef SENSOR_SOILMODBUS_DEBUG
      Serial.print(id); Serial.println(F(": no reply"));
    #endif
    #ifdef SENSOR_SOILMODBUS_AUTOPROVISION
      // A sector with no probe is the only reason to go looking for an unprovisioned one
      autoProvision();
    #endif
  } else {
    // Both registers are scaled by ten. Temperature is signed two's complement - without the
    // correction a probe below freezing reads as about +6500C.
    const float humy = raw[0] / 10.0f;
    const int16_t t_signed = (int16_t)raw[1];
    const float temp = t_signed / 10.0f;
    #ifdef SENSOR_SOILMODBUS_DEBUG
      Serial.print(id); Serial.print(F(" raw=")); Serial.print(raw[0]);
      Serial.print(F(",")); Serial.print(raw[1]);
      Serial.print(F(" -> ")); Serial.print(humy, 1); Serial.print(F("% "));
      Serial.print(temp, 1); Serial.println(F("C"));
    #endif
    if (validate(humy, temp)) {
      humidity->set(humy);
      temperature->set(temp);
    } else {
      setOutputsInvalid();
      #ifdef SENSOR_SOILMODBUS_DEBUG
        Serial.print(id); Serial.println(F(": reading failed validation"));
      #endif
    }
  }
}

#endif // SENSOR_SOILMODBUS_WANT
