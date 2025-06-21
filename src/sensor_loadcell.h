#ifndef SENSOR_LOADCELL_H
#define SENSOR_LOADCELL_H

/* Frugal IoT - load cell sensor
*/
#include "_settings.h"  // Settings for what to include etc
#ifdef SENSOR_LOADCELL_WANT

#include "sensor_float.h"
#include <HX711.h> // https://registry.platformio.org/libraries/robtillaart/HX711

class Sensor_LoadCell : public Sensor_Float {
  public:
    Sensor_LoadCell(const char* const id, const char * const name, float max, const char* color, const bool retain);
    float read();
    void tare();
    void setup();
    void calibrate(uint16_t weight);
  private:
    HX711 *hx711;
    int32_t offset;
    int32_t scale;
};
#endif // SENSOR_LOADCELL_WANT
#endif // SENSOR_LOADCELL_H