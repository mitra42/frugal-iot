/* Frugal IoT - Dissolved Oxygen sensor (analog, temperature compensated)
 *
 * NOTE - AS OF 2026-08-03 THIS IS UNTESTED CODE
 *
 * See sensor/dissolvedoxygen.h for the calibration flags and how this maps onto Sensor_Analog.
 */

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "sensor/dissolvedoxygen.h"
#include "Frugal-IoT.h" // For frugal_iot

// Dissolved oxygen concentration of air-saturated water, in ug/L, indexed by whole degrees C
// from 0 to 40. So 14460 means 14.46 mg/L at 0 C.
static const uint16_t DO_saturation_ugl[41] = {
  14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
  11260, 11010, 10770, 10530, 10300, 10080,  9860,  9660,  9460,  9270,
   9080,  8900,  8730,  8570,  8410,  8250,  8110,  7960,  7820,  7690,
   7560,  7430,  7300,  7180,  7070,  6950,  6840,  6730,  6630,  6530, 6410
};
#define DO_TABLE_MAX_T 40

Sensor_DissolvedOxygen::Sensor_DissolvedOxygen(const char* const id, const char * const name,
  const uint8_t pin, const char* color, const bool retain)
  //TODO-213 define min/max/color in the UX defaults (generate-defaults.js) - literals for now
  // width 2, 0..20 mg/L, offset 0 - the temperature compensation lives entirely in scale
  : Sensor_Analog(id, name, pin, 2, 0, 20, 0, scaleForTemperature(SENSOR_DO_DEFAULT_TEMPERATURE), color, retain),
    watertemperature(new INfloat(id, "temperature", "Water Temperature",
      SENSOR_DO_DEFAULT_TEMPERATURE, 1, -10, 50, "red", true))
  { }

// mg/L per mV. Combines the saturation lookup with the probe's saturation voltage at that
// temperature, and folds in the ug/L -> mg/L conversion.
float Sensor_DissolvedOxygen::scaleForTemperature(float temp_c) {
  // The original code cast the temperature to uint8_t and indexed the table unchecked, so
  // anything outside 0..40 C read past the end of the array - clamp instead.
  int t = (int)lroundf(temp_c);
  if (t < 0) {
    t = 0;
  } else if (t > DO_TABLE_MAX_T) {
    t = DO_TABLE_MAX_T;
  }
  #if defined(SENSOR_DO_CAL2_V) && defined(SENSOR_DO_CAL2_T)
    // Two-point: straight line through (CAL2_T, CAL2_V) and (CAL1_T, CAL1_V)
    float v_saturation = ((float)(t - SENSOR_DO_CAL2_T)
      * (float)(SENSOR_DO_CAL1_V - SENSOR_DO_CAL2_V)
      / (float)(SENSOR_DO_CAL1_T - SENSOR_DO_CAL2_T)) + (float)SENSOR_DO_CAL2_V;
  #else
    // Single point: the probe's saturation voltage rises 35mV per degree C
    float v_saturation = (float)SENSOR_DO_CAL1_V + 35.0 * (float)(t - SENSOR_DO_CAL1_T);
  #endif
  float s = 0.0;
  if (v_saturation > 0.0) { // Two-point calibration can extrapolate to zero or negative
    s = (float)DO_saturation_ugl[t] / (v_saturation * 1000.0);
  }
  #ifdef SENSOR_DO_DEBUG
    Serial.print(F("DO scale: T=")); Serial.print(t);
    Serial.print(F(" saturation=")); Serial.print(DO_saturation_ugl[t]);
    Serial.print(F("ug/L v_saturation=")); Serial.print(v_saturation);
    Serial.print(F("mV scale=")); Serial.println(s, 6);
  #endif
  return s;
}

void Sensor_DissolvedOxygen::setup() {
  watertemperature->setup(); // Before readConfigFromFS, which may carry a stored wired path
  Sensor_Analog::setup();    // pinMode, and Sensor::setup -> readConfigFromFS
  // Only apply the compile-time default if nothing on the filesystem or in the UX wired it
  if (!watertemperature->wiredPath.length()) {
    watertemperature->wireTo(frugal_iot.messages->path(SENSOR_DO_TEMPERATURE_PATH));
  }
  #ifdef SENSOR_DO_DEBUG
    Serial.print(F("DO water temperature wired to ")); Serial.println(watertemperature->wiredPath);
  #endif
}

void Sensor_DissolvedOxygen::discover() {
  Sensor::discover(); // Outputs
  watertemperature->discover();
}

void Sensor_DissolvedOxygen::dispatch(System_Message &msg) {
  // The wired water temperature is published by *another* module, so this has to happen
  // outside any msg.module() == id test - compare Control::dispatch(), which does the same.
  // Sensor::dispatch() wraps everything in that test, which is why this cannot just be
  // delegated upwards.
  if (watertemperature->dispatch(msg)) {
    // Note this deliberately overwrites any scale read from the filesystem or set over MQTT -
    // for this sensor scale is derived from temperature, not calibrated independently.
    scale = scaleForTemperature(watertemperature->floatValue());
  }
  Sensor_Analog::dispatch(msg);
}

// Probe voltage in millivolts. Sensor_Analog::convert() then applies (mv - 0) * scale.
int Sensor_DissolvedOxygen::readInt() {
  const int raw = Sensor_Analog::readInt(); // analogRead(pin)
  const int millivolts = (int)((uint32_t)SENSOR_DO_VREF * (uint32_t)raw / (uint32_t)SENSOR_DO_ADC_RES);
  #ifdef SENSOR_DO_DEBUG
    Serial.print(F("DO raw=")); Serial.print(raw);
    Serial.print(F(" mV=")); Serial.println(millivolts);
  #endif
  return millivolts;
}

bool Sensor_DissolvedOxygen::validate(int millivolts) {
  // Cannot read outside the ADC's range; anything else means a bad read rather than 0 mg/L
  return (millivolts >= 0) && (millivolts <= SENSOR_DO_VREF);
}

void Sensor_DissolvedOxygen::captiveLines(AsyncResponseStream* response) {
  // Deliberately not Sensor_Float::captiveLines() - that offers the reading as an editable
  // number, which for a Sensor_Analog means calibrate() and would set a scale that the next
  // temperature message immediately overwrites. Show it read-only instead.
  response->print(String(F("<p><label>")) + name
    + "<br>Dissolved Oxygen: " + output->StringValue() + " mg/L"
    + "<br>Water Temperature: " + watertemperature->StringValue() + " C</label></p>");
}
