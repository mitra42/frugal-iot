/*
 * GPS sensor reading NMEA 0183 sentences via UART (e.g. Quectel L76K).
 *
 * Mitra Ardron: 2026
 *
 * See Quectel L76K datasheet and Heltec WiFi LoRa 32 V4 schematic for wiring.
 *
 * Required library:
 *   mikalhart/TinyGPSPlus  (NMEA parser)
 *
 * Optional build flags (with defaults):
 *   SENSOR_GPS_BAUD            (9600)  — module UART baud rate
 *   SENSOR_GPS_READ_TIMEOUT_MS (1100)  — max ms to wait for a fresh fix after
 *                                        flushing the stale UART buffer; covers
 *                                        one full 1 Hz L76K update cycle + jitter
 *   SENSOR_GPS_3v3_PIN         (0xFF)  — GPIO driven HIGH to power the module
 *                                        (e.g. GPIO 46 on Heltec V4, active HIGH)
 *   SENSOR_GPS_0v_PIN          (0xFF)  — GPIO driven LOW for ground enable
 *                                        (not used on Heltec V4)
 *   SENSOR_GPS_DEBUG           — enable Serial debug output
 *
 * Outputs published to MQTT:
 *   gps/latitude   — decimal degrees, -90 to +90
 *   gps/longitude  — decimal degrees, -180 to +180
 *   gps/altitude   — metres above MSL
 *   gps/speed      — km/h (converted from knots)
 *   gps/course     — true heading, degrees 0–360
 *   gps/hdop       — horizontal dilution of precision
 *   gps/satellites — satellite count
 *   gps/position   — ISO 6709: "+lat+lon+alt/"
 *   gps/utc_time   — "HH:MM:SS"
 *
 * All outputs are only set when the module reports a valid fix.
 */

#ifndef SENSOR_GPS_H
#define SENSOR_GPS_H

#include "sensor.h"
#include <TinyGPSPlus.h>

#ifndef SENSOR_GPS_BAUD
  #define SENSOR_GPS_BAUD 9600
#endif

#ifndef SENSOR_GPS_READ_TIMEOUT_MS
  #define SENSOR_GPS_READ_TIMEOUT_MS 1100
#endif

#ifndef SENSOR_GPS_3v3_PIN
  #define SENSOR_GPS_3v3_PIN 0xFF
#endif
#ifndef SENSOR_GPS_0v_PIN
  #define SENSOR_GPS_0v_PIN 0xFF
#endif

// GPIO driven HIGH continuously to prevent the module from entering sleep
// between reads (e.g. GNSS_WAKE = GPIO 40 on Heltec V4). 0xFF = unused.
#ifndef SENSOR_GPS_WAKE_PIN
  #define SENSOR_GPS_WAKE_PIN 0xFF
#endif

// GPIO driven HIGH to release the module from hardware reset
// (e.g. GNSS_RST = GPIO 42 on Heltec V4). 0xFF = unused.
#ifndef SENSOR_GPS_RST_PIN
  #define SENSOR_GPS_RST_PIN 0xFF
#endif

class Sensor_GPS : public Sensor {
  public:
    Sensor_GPS(const char* const name,
               HardwareSerial* serial,
               uint8_t rx_pin,
               uint8_t tx_pin,
               uint32_t baud           = SENSOR_GPS_BAUD,
               bool retain             = true,
               uint8_t power3v3_pin    = SENSOR_GPS_3v3_PIN,
               uint8_t power0v_pin     = SENSOR_GPS_0v_PIN);

    OUTfloat*   latitude;
    OUTfloat*   longitude;
    OUTfloat*   altitude;   // metres above MSL
    OUTfloat*   speed;      // km/h, converted from NMEA knots
    OUTfloat*   course;     // true heading, 0–360°
    OUTfloat*   hdop;       // horizontal dilution of precision
    OUTuint16*  satellites;
    OUTtext*    position;   // ISO 6709: "+lat+lon+alt/"
    OUTtext*    utc_time;   // "HH:MM:SS"

  protected:
    HardwareSerial* _serial;
    uint8_t  _rx_pin;
    uint8_t  _tx_pin;
    uint32_t _baud;
    TinyGPSPlus _gps;

    void setup() override;
    void readValidateConvertSet() override;
};

#endif // SENSOR_GPS_H
