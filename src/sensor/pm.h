// Deep Sleep issues: a fan-and-laser particulate sensor must run for a warm-up before its readings mean anything, so okToSleep() holds the node awake from each periodically() until the reading is published - warm-up + reading time, about 20-35s a cycle. The measurement state is a plain member and restarts each wake, which is what we want; whether the module ran through the sleep is the subclass's to say, by setting `running` in setup().
/*
 * Sensor_PM - the part shared by laser particulate (PM) sensors: Nova SDS011, Seeed HM3301, and in
 * time Plantower PMS5003/7003, Sensirion SPS30 ... A base class only - see sensor/sds011.h and
 * sensor/hm3301.h for the sensors themselves.
 *
 * NOTE - AS OF 2026-11-06 THIS IS UNTESTED ON HARDWARE.
 *
 * What every one of these sensors has in common, and so what lives here:
 *   A fan draws air past a laser. After the fan starts, it needs a warm-up (15-30s depending on the
 *   maker) before the chamber holds the air outside rather than whatever sat in it. Then readings,
 *   about one a second, are noisy enough to be worth averaging.
 *
 * So a measurement is a small state machine, driven by periodically() and loop():
 *   periodically()  - wake the module (unless it never slept), start the warm-up, and start
 *                     refusing sleep (okToSleep)
 *   WARMING         - warmup_ms: samples are read (a UART must be drained) but discarded
 *   READING         - reading_ms: every good sample is added up
 *   publish         - mean of the samples, dropping the highest and lowest when there are 3+ (as
 *                     sensor.community's airRohr does); NAN if none arrived (module missing or
 *                     miswired). Then sleep the module, and allow the node to sleep.
 * With Power_Deep the node is therefore awake for about warmup+reading each cycle, whatever
 * wake_ms says. If cycle_ms is shorter than that the module is left running, and each cycle after
 * the first just reads.
 *
 * What differs between sensors, and so what a subclass provides:
 *   wakeModule()     - start the fan and laser (a command, or nothing if it cannot sleep)
 *   sleepModule()    - stop them; return false if this module cannot be slept
 *   readSample(v)    - called on every loop() pass in every state. Return true with a new sample
 *                      in v[PM1], v[PM25], v[PM10], in TENTHS of ug/m3. Leave a channel the
 *                      sensor does not measure alone. A polled sensor should pace itself (see
 *                      Sensor_HM3301), a streaming one just drains what has arrived.
 *   reportNoData()   - print a hint for the commonest wiring fault
 *   the outputs      - set pm1 / pm25 / pm10 to OUTfloats in the constructor; leave a channel the
 *                      sensor does not measure as nullptr and it is skipped throughout
 *   running          - set it in setup() if the module is already running at boot, e.g. it cannot
 *                      be slept and this is a deep-sleep wake - then the first measurement skips
 *                      the warm-up
 *
 * Accuracy: all of these are unreliable above ~70% relative humidity, as water droplets are counted
 * as particles. Nothing here corrects for it - examples/particulates publishes humidity alongside.
 *
 * Build flags: SENSOR_SDS011_DEBUG and SENSOR_HM3301_DEBUG both turn on this class's debug output.
 */

#ifndef SENSOR_PM_H
#define SENSOR_PM_H

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "sensor/sensor.h"

#if defined(SENSOR_SDS011_DEBUG) || defined(SENSOR_HM3301_DEBUG)
  #define SENSOR_PM_DEBUG
#endif

class Sensor_PM : public Sensor {
  public:
    Sensor_PM(const char * const id, const char * const name, unsigned long warmup_ms,
      unsigned long reading_ms, bool retain);
    // Each nullptr unless the subclass's sensor measures it - see the top of this file
    OUTfloat* pm1 = nullptr;
    OUTfloat* pm25 = nullptr;
    OUTfloat* pm10 = nullptr;
    void prepare() override;  // Sleep the module, if a sleep catches it running
  protected:
    enum Channel : uint8_t { PM1, PM25, PM10, CHANNELS };
    enum State : uint8_t { IDLE, WARMING, READING };
    const unsigned long warmup_ms;
    const unsigned long reading_ms;
    State state = IDLE;
    bool running = false;      // Whether the module has been woken and not since slept
    unsigned long phaseStart;  // millis() the current WARMING or READING phase began
    // In tenths of ug/m3 - 30 samples of at most 9999 fit easily
    uint32_t sum[CHANNELS];
    uint16_t minimum[CHANNELS];
    uint16_t maximum[CHANNELS];
    uint8_t count;

    void loop() override;
    void periodically() override;            // Starts a measurement - does NOT read
    void readValidateConvertSet() override;  // Publishes the measurement that loop() collected
    bool okToSleep(unsigned long ms, uint16_t how) override;
    OUTfloat* channel(uint8_t c);  // pm1, pm25 or pm10
    void wake();
    void sleep();
    bool keepRunning();  // Cycle too short to be worth sleeping the module

    // The subclass's part - see the top of this file
    virtual void wakeModule() = 0;
    virtual bool sleepModule() = 0;
    virtual bool readSample(uint16_t* v) = 0;
    virtual void reportNoData() = 0;
};

#endif // SENSOR_PM_H
