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
    reg(reg)
  {
    humidity->unit = "%";
    temperature->unit = "C";
    outputs.push_back(humidity);
    outputs.push_back(temperature);
  }

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

void Sensor_SoilModbus::readValidateConvertSet() {
  uint16_t raw[SENSOR_SOILMODBUS_COUNT] = {0, 0};
  if (!modbus.readRegisters(reg, SENSOR_SOILMODBUS_COUNT, raw)) {
    // No answer. Say so rather than leaving the previous reading standing - a control deciding
    // whether to open a valve needs to know the difference between "still 40%" and "no idea".
    setOutputsInvalid();
    #ifdef SENSOR_SOILMODBUS_DEBUG
      Serial.print(id); Serial.println(F(": no reply"));
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
