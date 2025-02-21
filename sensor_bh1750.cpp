/* 
 * BH1750 light sensor as in Lilygo HiGrow
 * 
 * Optional: SENSOR_BH1750_ADDRESS
 */

#include "_settings.h"

#ifdef SENSOR_BH1750_WANT
#include "sensor_bh1750.h"

#include <BH1750.h>             //https://github.com/claws/BH1750


Sensor_BH1750::Sensor_BH1750(const char* topic, uint8_t pin, const unsigned long ms, bool retain)
  : Sensor_Float(topic, 3, ms, retain), pin(pin), lightmeter(pin) {
  }

void Sensor_BH1750::setup() {
  if (!lightmeter.begin()) {
    Serial.println("Warning: Failed to initialize BH1750 light sensor!");
  } else {
    Serial.println("BH1750 light sensor init succeeded");
  }
}
float Sensor_BH1750::read() {
  float v;
  v = lightmeter.readLightLevel();
  if (isnan(v)) {
      v = 0.0;
  }
  return v;
}

#endif // SENSOR_BH1750_WANT
