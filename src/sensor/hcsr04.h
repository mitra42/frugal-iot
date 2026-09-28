// Deep Sleep issues: none - nothing is carried between reads, and setup() sets the pins up again each wake.
/*
 * HC-SR04 ultrasonic distance sensor - trigger / echo timing.
 *
 * NOTE - AS OF 2026-09-28 THIS IS UNTESTED CODE
 *
 * Freestanding - no external library. robtillaart/SRF05 supports this module, but was looked at
 * and not used: a timed-out ping returns 0 and its average/median modes fold those zeros into
 * the result, its pulseIn timeout is fixed at 200ms, its default 1ms between samples is far
 * below the 60ms this module needs, and an echo pin of 0 means "single pin mode", so GPIO0
 * cannot be the echo pin. The measurement itself is a dozen lines, below.
 *
 * How it measures: a 10us pulse on TRIG makes the module send a 40kHz burst and then hold ECHO
 * high until the echo returns. The time ECHO is high is the round trip, so
 *     distance_mm = echo_us * speed_of_sound_m_per_s / 2000
 * Each read takes SENSOR_HCSR04_SAMPLES pings, SENSOR_HCSR04_INTERVAL_MS apart (the module needs
 * 60ms for a stray echo from the last ping to die away), drops the ones that timed out, and
 * publishes the median of the rest - or "nan" if none came back. Readings outside the module's
 * rated 20..4000mm are rejected too.
 *
 * Temperature: the speed of sound is 331.3 + 0.606 * T m/s, about 0.17% per degree C - 3.4cm at
 * 2m for a 10 degree error, which matters for the depth of a tank. So there is a wireable
 * `temperature` input. Wire it to any temperature sensor on the node or elsewhere, in the sketch,
 * the UX or with SENSOR_HCSR04_TEMPERATURE_PATH. Until something arrives on it,
 * SENSOR_HCSR04_DEFAULT_TEMPERATURE is used.
 *
 * WIRING - the standard HC-SR04 is a 5V part: VCC to 5V, and ECHO drives 5V, which an ESP32 or
 * ESP8266 pin is not rated for. Put a divider on ECHO - e.g. 1k in series from ECHO to the pin,
 * and 2k from the pin to GND. TRIG is fine driven at 3.3V. The HC-SR04P (and RCWL-9610 based
 * boards) run at 3.3V and need no divider.
 *
 * Blocking: roughly 0.3s per read at the defaults, a little more if the module is missing, as
 * each ping waits SENSOR_HCSR04_TIMEOUT_US.
 *
 * Output published to MQTT - see sensor/ultrasonic.h for offset and scale:
 *   <id>/<id>          - distance in mm (offset + raw * scale)
 * Input:
 *   <id>/temperature   - air temperature in C, wireable
 *
 * Build flags:
 *   SENSOR_HCSR04_TRIGGER_PIN, SENSOR_HCSR04_ECHO_PIN - not used by the library, which takes the
 *                                     pins as constructor arguments; the examples pass these
 *   SENSOR_HCSR04_SAMPLES        (5)     - pings per read, median taken
 *   SENSOR_HCSR04_INTERVAL_MS    (60)    - wait between pings
 *   SENSOR_HCSR04_TIMEOUT_US     (30000) - give up on an echo (30ms is about 5m)
 *   SENSOR_HCSR04_MIN_MM (20), SENSOR_HCSR04_MAX_MM (4000) - valid range
 *   SENSOR_HCSR04_DEFAULT_TEMPERATURE (20) - C, until the temperature input has a value
 *   SENSOR_HCSR04_TEMPERATURE_PATH - default wiring for the temperature input, e.g. "sht/temperature"
 *   SENSOR_HCSR04_DEBUG            - Serial debug output
 */

#ifndef SENSOR_HCSR04_H
#define SENSOR_HCSR04_H

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "sensor/ultrasonic.h"

#ifndef SENSOR_HCSR04_SAMPLES
  #define SENSOR_HCSR04_SAMPLES 5
#endif
#ifndef SENSOR_HCSR04_INTERVAL_MS
  #define SENSOR_HCSR04_INTERVAL_MS 60
#endif
#ifndef SENSOR_HCSR04_TIMEOUT_US
  #define SENSOR_HCSR04_TIMEOUT_US 30000
#endif
#ifndef SENSOR_HCSR04_MIN_MM
  #define SENSOR_HCSR04_MIN_MM 20
#endif
#ifndef SENSOR_HCSR04_MAX_MM
  #define SENSOR_HCSR04_MAX_MM 4000
#endif
#ifndef SENSOR_HCSR04_DEFAULT_TEMPERATURE
  #define SENSOR_HCSR04_DEFAULT_TEMPERATURE 20.0
#endif

// TODO - not in the server's modules.yaml yet, so defaults.h has no DEFAULT_hcsr04_* macros.
// These stand in until it does, and then defaults.h (included via _settings.h) takes over.
#ifndef DEFAULT_hcsr04_hcsr04_color
  #define DEFAULT_hcsr04_hcsr04_color "#0000ff"
#endif
#ifndef DEFAULT_hcsr04_hcsr04_max
  #define DEFAULT_hcsr04_hcsr04_max 4000
#endif

class Sensor_HCSR04 : public Sensor_Ultrasonic {
  public:
    Sensor_HCSR04(const char* const id, const char * const name, uint8_t trigger_pin, uint8_t echo_pin,
      float max = DEFAULT_hcsr04_hcsr04_max, const char* color = DEFAULT_hcsr04_hcsr04_color,
      const bool retain = true, float offset = 0.0, float scale = 1.0);
    INfloat* temperature; // Air temperature, for the speed of sound - public so a sketch can wireTo it
  protected:
    const uint8_t trigger_pin;
    const uint8_t echo_pin;
    void setup() override;
    void discover() override;
    void dispatch(System_Message &msg) override;
    float readFloat() override;       // Median distance in mm, or NAN if no echo came back
    bool validate(float mm) override; // Within the module's rated range
    uint32_t ping();                  // One round trip in us, 0 if it timed out
    float speedOfSound();             // m/s at the current temperature input
};

#endif // SENSOR_HCSR04_H
