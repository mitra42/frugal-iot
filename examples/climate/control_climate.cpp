/* Frugal IoT - Control_Climate - Dual-channel hysteresis for temperature and humidity control.
 *
 * Follows the same pattern as Control_Hysterisis but with two independent channels.
 *
 * Inputs (INfloat):
 *   0: temperature         - current temp reading in C (wireable, wired to sensor)
 *   1: temp_setpoint       - target temperature in C
 *   2: temp_hysteresis     - temperature deadband in C
 *   3: humidity            - current humidity reading in % (wireable, wired to sensor)
 *   4: humidity_setpoint   - target humidity in %
 *   5: humidity_hysteresis - humidity deadband in %
 *
 * Outputs (OUTbool):
 *   0: temp_out     - drives heating actuator (ON when too cold)
 *   1: humidity_out - drives humidifier actuator (ON when too dry)
 */

#include "_settings.h"  // Settings for what to include etc

#include <Arduino.h>
#include "misc.h"
#include "control_climate.h"

#ifdef CONTROL_CLIMATE_DEBUG
void Control_Climate::debug(const char* const where) {
  Serial.printf("%s: ", where);
  Serial.printf(" temp=%f", inputs[0]->floatValue());
  Serial.printf(" temp_setpoint=%f", inputs[1]->floatValue());
  Serial.printf(" temp_hyst=%f", inputs[2]->floatValue());
  Serial.printf(" humidity=%f", inputs[3]->floatValue());
  Serial.printf(" humidity_setpoint=%f", inputs[4]->floatValue());
  Serial.printf(" humidity_hyst=%f", inputs[5]->floatValue());
  Serial.printf(" temp_out=%d", outputs[0]->boolValue());
  Serial.printf(" humidity_out=%d", outputs[1]->boolValue());
  Serial.println();
}
#endif //CONTROL_CLIMATE_DEBUG

void Control_Climate::act() {
  // Temperature channel — hysteresis logic (same as Control_Hysterisis::act)
  // ON when temp falls below setpoint-hysteresis (too cold, needs heating)
  // OFF when temp rises above setpoint+hysteresis (warm enough)
  // No change within the deadband to avoid relay chatter
  const float temp = inputs[0]->floatValue();
  const float temp_setpoint = inputs[1]->floatValue();
  const float temp_hyst = inputs[2]->floatValue();
  if (temp < (temp_setpoint - temp_hyst)) {
    ((OUTbool*)outputs[0])->set(true);
  }
  if (temp > (temp_setpoint + temp_hyst)) {
    ((OUTbool*)outputs[0])->set(false);
  }

  // Humidity channel — same hysteresis logic
  // ON when humidity falls below setpoint-hysteresis (too dry, needs humidifying)
  // OFF when humidity rises above setpoint+hysteresis (humid enough)
  const float hum = inputs[3]->floatValue();
  const float hum_setpoint = inputs[4]->floatValue();
  const float hum_hyst = inputs[5]->floatValue();
  if (hum < (hum_setpoint - hum_hyst)) {
    ((OUTbool*)outputs[1])->set(true);
  }
  if (hum > (hum_setpoint + hum_hyst)) {
    ((OUTbool*)outputs[1])->set(false);
  }

  #ifdef CONTROL_CLIMATE_DEBUG
    debug("Control_Climate after act");
  #endif
};

Control_Climate::Control_Climate(const char* const id, const char* const name,
  float temp_setpoint, float temp_hysteresis,
  float humidity_setpoint, float humidity_hysteresis)
: Control(id, name,
  std::vector<IN*> {
    new INfloat(id, "temperature", "Temperature", temp_setpoint, 1, -40, 80, "black", true),
    new INfloat(id, "temp_setpoint", "Temp Setpoint", temp_setpoint, 1, -40, 80, "black", false),
    new INfloat(id, "temp_hysteresis", "Temp Hysteresis", temp_hysteresis, 1, 0, 20, "black", false),
    new INfloat(id, "humidity", "Humidity", humidity_setpoint, 1, 0, 100, "black", true),
    new INfloat(id, "humidity_setpoint", "Humidity Setpoint", humidity_setpoint, 1, 0, 100, "black", false),
    new INfloat(id, "humidity_hysteresis", "Humidity Hysteresis", humidity_hysteresis, 1, 0, 50, "black", false)
  },
  std::vector<OUT*> {
    new OUTbool(id, "temp_out", "Heating", false, "black", true),
    new OUTbool(id, "humidity_out", "Humidifier", false, "black", true)
  }
) {
  #ifdef CONTROL_CLIMATE_DEBUG
    debug("Control_Climate after instantiation");
  #endif
};
