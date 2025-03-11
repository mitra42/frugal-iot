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
  Wire.begin(I2C_SDA, I2C_SCL); // Note potential conflict with I2C on SHT.
  if (!lightmeter.begin()) {
    Serial.println("Warning: Failed to initialize BH1750 light sensor!"); delay(5000);
  } else {
    Serial.println("BH1750 light sensor init succeeded"); delay(5000);
  }
}
float Sensor_BH1750::read() {
  float v;
  v = lightmeter.readLightLevel();
  if (isnan(v)) {
      v = 0.0;
  }
  #ifdef SENSOR_BH1750_DEBUG
    Serial.print("BH1750:"); Serial.println(v);
  #endif
  return v;
}

#endif // SENSOR_BH1750_WANT
