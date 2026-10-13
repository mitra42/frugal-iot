// Deep Sleep issues: none - nothing is carried between reads, and setup() starts the UART again each wake.
/*
 * DFRobot A01NYUB (SEN0313) waterproof ultrasonic distance sensor - UART output.
 *
 * NOTE - AS OF 2026-10-13 THIS IS UNTESTED CODE, we do not have the hardware
 *
 * Freestanding - no external library, the protocol is four bytes.
 *
 * How it reports: the module sends a frame on its TX line continuously, 9600 baud 8N1:
 *     0xFF  DATA_H  DATA_L  SUM        distance_mm = DATA_H * 256 + DATA_L
 *                                      SUM = (0xFF + DATA_H + DATA_L) & 0xFF
 * DATA_H is never 0xFF (7500mm is 0x1D4C) but DATA_L can be, so a 0xFF alone does not mark the
 * start of a frame - readFloat() slides a four byte window along and only accepts a window whose
 * checksum matches.
 *
 * Each read discards whatever is already buffered (stale for any cycle longer than a few hundred
 * ms), then waits up to SENSOR_A01NYUB_TIMEOUT_MS for the first good frame and returns it, or
 * NAN if none came - which publishes "nan". Readings outside SENSOR_A01NYUB_MIN_MM ..
 * SENSOR_A01NYUB_MAX_MM are rejected by validate().
 *
 * There is no median here, unlike Sensor_HCSR04 - in its default mode the module filters its
 * own readings (see the RX pin, below). And no temperature input: the module does its own.
 * TODO - check on hardware that it really does compensate for temperature; DFRobot's
 * A02YYUW sibling does, the A01NYUB documentation seen so far does not say.
 *
 * Module (from DFRobot's wiki and the ESPHome / Zephyr drivers, not yet checked on hardware):
 *   Range       28cm - 7.5m. Closer than 28cm is the blind zone: one transducer sends AND
 *               listens, and cannot hear while it is still ringing.
 *   Supply      3.3 - 5V. TX follows VCC, so power it from 3V3 and no divider is needed;
 *               from 5V, put a divider on TX as for an HC-SR04's ECHO (1k series, 2k to GND).
 *   Cable       Red VCC, Black GND, Yellow RX (mode select), White TX (data out) - but check
 *               the label on your unit, cable colours vary between batches of these modules.
 *   RX pin      Not a serial input - it selects the output mode:
 *                 floating or HIGH - "processed" value, filtered, a frame every 150-300ms (default)
 *                 LOW              - "real-time" value, unfiltered, a frame about every 150ms
 *               Leave it unconnected for the processed mode, which is what this class expects.
 *
 * Related modules - the A01NYUB is one of a family; only the UART auto-output ones fit this class:
 *   A02YYUW  - 3cm - 4.5m, same frame - should work with SENSOR_A01NYUB_MIN_MM=30, _MAX_MM=4500
 *   A01NYUB  - this one
 *   A01ANY4B - RS485/Modbus version, see Sensor_UltrasonicModbus
 *   "UART controlled" variants send a frame only when RX is pulsed - would need a new subclass
 *   AJ-SR04M in its serial modes (R19 = 120k) uses a different frame - see sensor/hcsr04.h
 *
 * The limits are build flags, so they apply to every Sensor_A01NYUB on the node.
 *
 * UART: pass the HardwareSerial and the ESP pin the module's TX goes to. No ESP TX pin is
 * claimed. On ESP32 any free UART works - Serial1 on the C3 and S2, which have no Serial2.
 * ESP8266 is not practical: UART0 is the console and Serial1 can only transmit; the rx_pin is
 * ignored there, as in Sensor_GPS.
 *
 * Blocking: up to SENSOR_A01NYUB_TIMEOUT_MS per read when the module is missing; normally one
 * frame interval, 150-300ms.
 *
 * Output published to MQTT - see sensor/ultrasonic.h for offset and scale:
 *   <id>/<id>          - distance in mm (offset + raw * scale)
 *
 * Build flags:
 *   SENSOR_A01NYUB_RX_PIN - not used by the library, which takes the pin as a constructor
 *                           argument; for sketches to pass
 *   SENSOR_A01NYUB_TIMEOUT_MS (1000) - wait for a good frame after the buffer is flushed
 *   SENSOR_A01NYUB_MIN_MM (280), SENSOR_A01NYUB_MAX_MM (7500) - valid range
 *   SENSOR_A01NYUB_DEBUG             - Serial debug output
 */

#ifndef SENSOR_A01NYUB_H
#define SENSOR_A01NYUB_H

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include "sensor/ultrasonic.h"

#ifndef SENSOR_A01NYUB_TIMEOUT_MS
  #define SENSOR_A01NYUB_TIMEOUT_MS 1000 // Over three of the slowest (300ms) processed-mode frames
#endif
#ifndef SENSOR_A01NYUB_MIN_MM
  #define SENSOR_A01NYUB_MIN_MM 280
#endif
#ifndef SENSOR_A01NYUB_MAX_MM
  #define SENSOR_A01NYUB_MAX_MM 7500
#endif

// TODO - not in the server's modules.yaml yet, so defaults.h has no DEFAULT_a01nyub_* macros.
// These stand in until it does, and then defaults.h (included via _settings.h) takes over.
#ifndef DEFAULT_a01nyub_a01nyub_color
  #define DEFAULT_a01nyub_a01nyub_color "#0000ff"
#endif
#ifndef DEFAULT_a01nyub_a01nyub_max
  #define DEFAULT_a01nyub_a01nyub_max 7500
#endif

class Sensor_A01NYUB : public Sensor_Ultrasonic {
  public:
    Sensor_A01NYUB(const char* const id, const char * const name, HardwareSerial* serial, uint8_t rx_pin,
      float max = DEFAULT_a01nyub_a01nyub_max, const char* color = DEFAULT_a01nyub_a01nyub_color,
      const bool retain = true, float offset = 0.0, float scale = 1.0);
  protected:
    HardwareSerial* serial;
    const uint8_t rx_pin;
    void setup() override;
    float readFloat() override;       // Distance in mm from the first good frame, or NAN
    bool validate(float mm) override; // Within the module's rated range
};

#endif // SENSOR_A01NYUB_H
