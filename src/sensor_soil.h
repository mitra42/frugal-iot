#ifndef SENSOR_SOIL_H
#define SENSOR_SOIL_H


#ifndef SENSOR_SOIL_POWER3v3_PIN
  #define SENSOR_SOIL_POWER3v3_PIN 0xFF
#endif
#ifndef SENSOR_SOIL_POWER0_PIN
  #define SENSOR_SOIL_POWER0_PIN 0xFF
#endif


#include "sensor_analog.h"

class Sensor_Soil : public Sensor_Analog {
  public: 
    Sensor_Soil(const char* const id, const char * const name, uint8_t pin_init, int offset, float scale, const char* color, bool retain, uint8_t power3v3_pin = SENSOR_SOIL_POWER3v3_PIN, uint8_t power0v_pin = SENSOR_SOIL_POWER0_PIN);
  protected:
    bool validate(int v) override;
    void captiveLines(AsyncResponseStream* response) override;
};

#endif // SENSOR_SOIL_H
