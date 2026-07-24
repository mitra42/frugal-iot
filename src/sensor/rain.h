/* Frugal IoT - Rain Sensor
 *
 * Reads an LM393-based analog rain board (e.g. YL-83/FC-37 "raindrop" modules) and
 * accumulates a running rain total, in the spirit of a bucket you periodically empty.
 *
 * These boards read close to their ADC max when dry, falling as the plate gets wetter.
 * There's no trustworthy published calibration curve from that analog reading to physical
 * rainfall - checked against other open-source projects/libraries, none exist, and the
 * physics (drop coalescence/drainage on the plate) argues against a stable one. So this
 * treats the raw reading as a relative wetness rate and integrates it over time, assuming
 * that relationship is linear - which is a simplification, not a calibrated fact.
 *
 * Usage: call tare() (or send "0" to the "output" topic, e.g. via the captive portal Tare
 * button) while the board is dry, to learn the dry baseline and zero the accumulated total.
 * Then, once you know the real rainfall over the same period (e.g. from a manual gauge
 * alongside), call calibrate(mm) (or send mm to "output") to set the scale.
 *
 * Note: accumulated total is a plain member, not RTC_DATA_ATTR - it resets on reboot/deep
 * sleep, same as it would with a physical gauge if power-cycled. Fine for Power_Loop; if used
 * with Power_Deep this will need revisiting.
 *
 * Configuration: SENSOR_RAIN_DEBUG
 */
#ifndef SENSOR_RAIN_H
#define SENSOR_RAIN_H

#include "_settings.h"  // Settings for what to include etc
#include "sensor/float.h"

class Sensor_Rain : public Sensor_Float {
  public:
    Sensor_Rain(const char* const id, const char * const name, uint8_t pin, float max, float scale, const char* color, bool retain);
  protected:
    float readFloat() override;
    void setup() override;
    void tare();
    void calibrate(float mm);
    void dispatch(System_Message &msg) override;
    void captiveLines(AsyncResponseStream* response) override;
    uint8_t pin;
    float scale;             // Converts accumulated raw wetness*seconds to mm - set by calibrate()
    float accumulated;       // Running raw wetness*seconds total since last tare()
    int dry_baseline;        // Raw analogRead() value learned as "dry" - set by tare()
    unsigned long last_read_ms;
}; // Class Sensor_Rain
#endif // SENSOR_RAIN_H
