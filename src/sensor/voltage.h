/* Sensor_Voltage - a DC voltage read through a resistor divider, reported in millivolts.
 *
 * Sensor_Battery was this class with its id hard-coded to "battery". A node often has more than
 * one voltage worth watching - a battery AND a solar panel, say - so the general case moved here
 * and Sensor_Battery became a thin subclass that keeps the battery-specific parts: the board
 * defaults for the pin and divider, its power-control pins, and being the instance that
 * System_Power::checkLevel() consults.
 *
 * ---------------------------------------------------------------------------------------------
 * Linearity, and why this differs from the rest of Sensor_Analog
 *
 * Sensor_Analog::readInt() returns analogRead() - RAW counts, with the ESP32 ADC's S-shaped
 * error left in. This class overrides it with analogReadMilliVolts(), which applies the chip's
 * FACTORY eFuse calibration and is close to linear across the usable range.
 *
 * That matters when porting readings from code that did its own correction. OSPIT, for instance,
 * reads raw counts and multiplies by a calibrated Vref, then applies a correction factor that is
 * itself a function of the raw value:
 *
 *     Voutcorrectionfactor = 1 + ((3150 - raw) * 0.000037)
 *
 * That is compensating for exactly the nonlinearity analogReadMilliVolts() has already removed.
 * Copying such a factor on top of this class would ADD error rather than remove it. Start with no
 * correction, measure against a meter, and only then decide whether a residual curve is worth
 * fitting - by overriding convert(), which is virtual for that purpose.
 *
 * ---------------------------------------------------------------------------------------------
 * divider and offset
 *
 * `voltage_divider` is the ratio between the voltage you want and the voltage at the pin: two
 * equal resistors give 2, a 1k/15k pair gives 16.
 *
 * `offset` is subtracted from the PIN reading before scaling, in millivolts at the pin, so a
 * constant error further up the chain has to be divided down to get here. A series Schottky
 * dropping 300 mV ahead of a 16:1 divider is offset = -300/16 = -19: "the pin reads 19 mV low".
 * Negative offsets are normal for this reason.
 *
 * It is also what Sensor_Analog's Tare button writes - taring says "whatever I am reading now is
 * really zero". That is usually what you want, but note it replaces any offset set here, and for
 * a diode it records the drop AT THE CURRENT FLOWING WHEN YOU PRESSED IT - a Schottky's forward
 * voltage is not constant. Tare in the condition you care about.
 */

#ifndef SENSOR_VOLTAGE_H
#define SENSOR_VOLTAGE_H

#include "sensor/analog.h"

class Sensor_Voltage : public Sensor_Analog {
  public:
    Sensor_Voltage(const char* const id, const char* const name, const uint8_t pin,
                   float voltage_divider, float min, float max,
                   int offset = 0, const char* color = DEFAULT_battery_battery_color,
                   bool retain = true);
  protected:
    #ifdef ESP32
      // Calibrated millivolts, not raw counts - see "Linearity" above
      int readInt() override;
    #endif
};

#endif // SENSOR_VOLTAGE_H
