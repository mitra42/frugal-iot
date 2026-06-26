/*
 * GPS sensor reading NMEA 0183 sentences via UART (e.g. Quectel L76K).
 *
 * Mitra Ardron: 2026
 *
 * UART buffer and blocking-read tradeoff
 * ──────────────────────────────────────
 * The ESP32 Arduino UART software ring buffer defaults to 256 bytes. At 9600 baud
 * the L76K fills it in ~260 ms, after which new bytes overwrite old ones (ring wrap).
 * Rather than draining continuously in every loop() iteration (which would require
 * framework changes), readValidateConvertSet():
 *
 *   1. Discards all currently-buffered bytes  — these are stale for any cycle longer
 *      than ~260 ms and for fast cycles they are nearly empty anyway.
 *   2. Blocks for up to SENSOR_GPS_READ_TIMEOUT_MS (default 1100 ms) waiting for
 *      TinyGPSPlus to decode a complete, valid GGA+RMC update.
 *
 * Tradeoff: this blocking call delays the entire frugal_iot loop() for up to 1.1 s
 * each reading cycle. For a GPS-only node this is fine. If other sensors or controls
 * need fast response, reduce SENSOR_GPS_READ_TIMEOUT_MS or move GPS to a separate
 * FreeRTOS task — out of scope here.
 *
 * Deep-sleep note
 * ───────────────
 * After deep sleep the GPS must re-acquire a fix (warm start: ~2–5 s; cold: up to
 * 60 s). Deep-sleep support is intentionally omitted; SENSOR_GPS_READ_TIMEOUT_MS is
 * not long enough for cold or warm reacquisition.
 */

#include "_settings.h"
#include <Arduino.h>
#include "sensor_gps.h"

Sensor_GPS::Sensor_GPS(const char* const name,
                       HardwareSerial* serial,
                       uint8_t rx_pin,
                       uint8_t tx_pin,
                       uint32_t baud,
                       bool retain,
                       uint8_t power3v3_pin,
                       uint8_t power0v_pin)
  : Sensor("gps", name, retain, power3v3_pin, power0v_pin),
    latitude(new OUTfloat(  "gps", "latitude",   "Latitude",   0, 6, -90.0f,   90.0f,   "blue",   false)),
    longitude(new OUTfloat( "gps", "longitude",  "Longitude",  0, 6, -180.0f,  180.0f,  "blue",   false)),
    altitude(new OUTfloat(  "gps", "altitude",   "Altitude",   0, 1, -500.0f,  9000.0f, "green",  false)),
    speed(new OUTfloat(     "gps", "speed",      "Speed",      0, 1,    0.0f,   999.0f, "orange", false)),
    course(new OUTfloat(    "gps", "course",     "Course",     0, 1,    0.0f,   360.0f, "black",  false)),
    hdop(new OUTfloat(      "gps", "hdop",       "HDOP",       0, 2,    0.0f,    50.0f, "black",  false)),
    satellites(new OUTuint16("gps", "satellites", "Satellites", 0,    0,      32,  "black",  false)),
    position(new OUTtext(   "gps", "position",   "Position",  "",            "black",  false)),
    utc_time(new OUTtext(   "gps", "utc_time",   "UTC Time",  "",            "black",  false)),
    _serial(serial), _rx_pin(rx_pin), _tx_pin(tx_pin), _baud(baud)
{
  outputs.push_back(latitude);
  outputs.push_back(longitude);
  outputs.push_back(altitude);
  outputs.push_back(speed);
  outputs.push_back(course);
  outputs.push_back(hdop);
  outputs.push_back(satellites);
  outputs.push_back(position);
  outputs.push_back(utc_time);
}

void Sensor_GPS::setup() {
  Sensor::setup(); // calls powerUp() — drives power0v_pin LOW (or power3v3_pin HIGH)

  // GNSS_WAKE: keep HIGH so the module never enters sleep between reads.
  // On Heltec V4 this is GPIO 40; 0xFF means the board does not have this pin.
  if (SENSOR_GPS_WAKE_PIN != 0xFF) {
    pinMode(SENSOR_GPS_WAKE_PIN, OUTPUT);
    digitalWrite(SENSOR_GPS_WAKE_PIN, HIGH);
  }

  // GNSS_RST: active-LOW reset — drive HIGH to release the module from reset.
  // On Heltec V4 this is GPIO 42.
  if (SENSOR_GPS_RST_PIN != 0xFF) {
    pinMode(SENSOR_GPS_RST_PIN, OUTPUT);
    digitalWrite(SENSOR_GPS_RST_PIN, HIGH);
  }

  _serial->begin(_baud, SERIAL_8N1, _rx_pin, _tx_pin);

  #ifdef SENSOR_GPS_DEBUG
    Serial.print(F("GPS UART started rx="));  Serial.print(_rx_pin);
    Serial.print(F(" tx="));                  Serial.print(_tx_pin);
    Serial.print(F(" baud="));                Serial.println(_baud);
    if (SENSOR_GPS_WAKE_PIN != 0xFF) { Serial.print(F(" wake=")); Serial.print(SENSOR_GPS_WAKE_PIN); }
    if (SENSOR_GPS_RST_PIN  != 0xFF) { Serial.print(F(" rst="));  Serial.print(SENSOR_GPS_RST_PIN);  }
    Serial.println();
  #endif
}

void Sensor_GPS::readValidateConvertSet() {
  // Step 1 — mark the flush start time, then discard all buffered bytes.
  // start is captured before the flush so that any bytes arriving during
  // the flush (which may be mid-sentence) are also excluded by the age check.
  uint32_t start = millis();
  while (_serial->available()) {
    _serial->read();
  }

  // Step 2 — collect bytes until TinyGPSPlus commits a valid location whose age
  // is less than the time elapsed since start (proving it arrived post-flush),
  // or until the timeout expires (one 1 Hz GPS update cycle + margin).
  bool fixed = false;
  while (!fixed && (millis() - start < SENSOR_GPS_READ_TIMEOUT_MS)) {
    while (_serial->available()) {
      _gps.encode(_serial->read());
    }
    if (_gps.location.isValid() && _gps.location.age() < (millis() - start)) {
      fixed = true;
    }
  }

  if (fixed) {
    double lat = _gps.location.lat();
    double lon = _gps.location.lng();
    double alt = _gps.altitude.isValid() ? _gps.altitude.meters() : 0.0;

    latitude->set((float)lat);
    longitude->set((float)lon);
    altitude->set((float)alt);

    if (_gps.speed.isValid()) {
      speed->set((float)_gps.speed.kmph());
    }
    if (_gps.course.isValid()) {
      course->set((float)_gps.course.deg());
    }
    if (_gps.hdop.isValid()) {
      hdop->set((float)_gps.hdop.hdop());
    }
    if (_gps.satellites.isValid()) {
      satellites->set((uint16_t)_gps.satellites.value());
    }

    // ISO 6709 decimal-degree string: sign-prefixed lat, lon, alt, terminated "/"
    // Example: "+27.591600+086.564000+8850.0/"
    char posStr[36];
    snprintf(posStr, sizeof(posStr), "%+.6f%+.6f%+.1f/", lat, lon, alt);
    position->set(String(posStr));

    if (_gps.time.isValid()) {
      char timeStr[10];
      snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
               _gps.time.hour(), _gps.time.minute(), _gps.time.second());
      utc_time->set(String(timeStr));
    }

    #ifdef SENSOR_GPS_DEBUG
    {
      char _gps_dbg[72];
      snprintf(_gps_dbg, sizeof(_gps_dbg), "GPS lat=%.6f lon=%.6f alt=%.1f sats=%d",
               lat, lon, alt,
               _gps.satellites.isValid() ? (int)_gps.satellites.value() : -1);
      Serial.println(_gps_dbg);
    }
    #endif
  #ifdef SENSOR_GPS_DEBUG
  } else {
    Serial.println(F("GPS: no fix within timeout"));
  #endif
  }
}
