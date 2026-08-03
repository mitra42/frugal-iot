/* Frugal IoT - Dissolved Oxygen sensor (analog, temperature compensated)
 *
 * NOTE - AS OF 2026-08-03 THIS IS UNTESTED CODE
 *
 * An analog DO probe on an ADC pin, of the common "DFRobot SEN0237"-style kind.
 *
 * CREDIT - the saturation lookup table and the V_saturation formulae come, via the Sensor
 * Tambak water-quality node, from DFRobot's published example for that probe:
 *   https://wiki-colabs.commonroom.info/Sensor_Tambak   (the node this was written for)
 *   https://wiki.dfrobot.com/Gravity__Analog_Dissolved_Oxygen_Sensor_SKU_SEN0237
 * Both single-point and two-point calibration modes are theirs. What is new here is the
 * temperature input, the mapping onto Sensor_Analog's offset/scale, and bounds-checking the
 * table lookup (the original indexed it with an unchecked cast - see scaleForTemperature).
 *
 * Dissolved oxygen saturation depends strongly on water temperature, so the probe's millivolt
 * reading only means something once you know the temperature. This class therefore takes an
 * IN, wired by default to the water temperature sensor - the first Sensor in Frugal-IoT to
 * have an input rather than only outputs.
 *
 * How it maps onto Sensor_Analog
 * ──────────────────────────────
 * Sensor_Analog already publishes `(reading - offset) * scale`. The DO formula is
 *
 *     DO = voltage_mv * DO_saturation(T) / V_saturation(T)
 *
 * which is just a scale factor, so nothing needs to override convert():
 *   - readInt()  returns the probe voltage in millivolts (not raw ADC counts)
 *   - offset     is 0
 *   - scale      is recomputed as DO_saturation(T) / V_saturation(T) each time a new water
 *                temperature arrives on the input
 *
 * Output is mg/L. The lookup table is in ug/L (14460 = 14.46 mg/L at 0 C), so the scale
 * carries the /1000.
 *
 * Calibration - put the probe in air-saturated water at a known temperature, read the
 * millivolts, and set SENSOR_DO_CAL1_V / SENSOR_DO_CAL1_T to what you measured. For two-point
 * calibration also define SENSOR_DO_CAL2_V / SENSOR_DO_CAL2_T, which switches the V_saturation
 * formula to a straight line through the two points (CAL1 is the high temperature point).
 *
 * Build flags:
 *   SENSOR_DO_VREF              (3300)  - ADC full-scale reference in mV
 *   SENSOR_DO_ADC_RES           (4095)  - ADC full-scale counts
 *   SENSOR_DO_CAL1_V            (269)   - probe mV in saturated water at SENSOR_DO_CAL1_T
 *   SENSOR_DO_CAL1_T            (25)    - that calibration temperature, C
 *   SENSOR_DO_CAL2_V, SENSOR_DO_CAL2_T  - define BOTH for two-point calibration
 *   SENSOR_DO_TEMPERATURE_PATH  ("ds18b20/ds18b20") - default wiring for the water temperature
 *   SENSOR_DO_DEFAULT_TEMPERATURE (25)  - assumed until the first temperature message arrives
 *   SENSOR_DO_DEBUG                     - Serial debug output
 */

#ifndef SENSOR_DISSOLVEDOXYGEN_H
#define SENSOR_DISSOLVEDOXYGEN_H

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "sensor/analog.h"

#ifndef SENSOR_DO_VREF
  #define SENSOR_DO_VREF 3300
#endif
#ifndef SENSOR_DO_ADC_RES
  #define SENSOR_DO_ADC_RES 4095
#endif
#ifndef SENSOR_DO_CAL1_V
  #define SENSOR_DO_CAL1_V 269
#endif
#ifndef SENSOR_DO_CAL1_T
  #define SENSOR_DO_CAL1_T 25
#endif
// SENSOR_DO_CAL2_V and SENSOR_DO_CAL2_T are intentionally not defaulted - defining both is
// what selects two-point calibration.
#ifndef SENSOR_DO_TEMPERATURE_PATH
  #define SENSOR_DO_TEMPERATURE_PATH "ds18b20/ds18b20"
#endif
#ifndef SENSOR_DO_DEFAULT_TEMPERATURE
  #define SENSOR_DO_DEFAULT_TEMPERATURE 25.0 // TODO what is this and how used
#endif

class Sensor_DissolvedOxygen : public Sensor_Analog {
  public:
    Sensor_DissolvedOxygen(const char* const id, const char * const name, const uint8_t pin,
      const char* color = "blue", const bool retain = true);
    // Held as IN* not INfloat* deliberately: INfloat::dispatch() and ::discover() are
    // protected overrides, only reachable through the public IN declarations.
    INfloat* watertemperature;
  protected:
    void setup() override;
    void discover() override;
    void dispatch(System_Message &msg) override;
    int readInt() override;            // Probe voltage in mV, not raw ADC counts
    bool validate(int millivolts) override;
    void captiveLines(AsyncResponseStream* response) override;
    // mg/L per mV at a given water temperature - static so it can seed `scale` in the
    // constructor's initialiser list, before any instance exists.
    static float scaleForTemperature(float temp_c);
};

#endif // SENSOR_DISSOLVEDOXYGEN_H
