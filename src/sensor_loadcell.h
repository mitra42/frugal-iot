#ifndef SENSOR_LOADCELL_H
#define SENSOR_LOADCELL_H

/* Frugal IoT - load cell sensor
*/
#include "_settings.h"  // Settings for what to include etc
#ifdef SENSOR_LOADCELL_WANT

#include "sensor_float.h"
#include <HX711.h> // https://registry.platformio.org/libraries/robtillaart/HX711

#ifndef SENSOR_LOADCELL_MS
  #define SENSOR_LOADCELL_MS 3600000 // 1 Minute
#endif

class Sensor_LoadCell : public Sensor_Float {
  public:
    Sensor_LoadCell(const char* name, const unsigned long ms, const bool retain);
    float read();
    void tare();
    void setup();
    void calibrate(uint16_t weight);
    String advertisement();
  private:
    HX711 *hx711;
    int32_t offset;
    int32_t scale;
};
#endif // SENSOR_LOADCELL_WANT
#endif // SENSOR_LOADCELL_H