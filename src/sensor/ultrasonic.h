// Deep Sleep issues: none here - see the subclasses.
/*
 * Ultrasonic distance sensor - the part every kind of ultrasonic module shares.
 *
 * Not instantiated directly. The subclasses differ only in how they get a raw distance in mm
 * (readFloat()); everything that happens after that is here:
 *
 *   Sensor_UltrasonicModbus (sensor/ultrasonicmodbus.h) - RS485 / Modbus RTU module, e.g. A01ANY4B
 *   Sensor_HCSR04           (sensor/hcsr04.h)           - HC-SR04 trigger/echo timing module
 *
 * Output published to MQTT:
 *   <id>/<id>  - offset + raw * scale
 *
 * The raw value is in millimetres, so the defaults (offset 0, scale 1) publish the distance
 * from the sensor face down to the reflecting surface in mm. To publish the depth of water in a
 * tank of known height instead, set scale to -1 and offset to the height of the sensor above
 * the tank floor in mm. Both are persisted to LittleFS and settable over MQTT; offset is also
 * editable in the captive portal (scale is not - the portal's number fields are step=1 so a
 * fractional scale could not be typed into one).
 *
 * Build flags:
 *   SENSOR_ULTRASONIC_DEBUG - Serial debug output (SENSOR_HCSR04_DEBUG also turns it on)
 */

#ifndef SENSOR_ULTRASONIC_H
#define SENSOR_ULTRASONIC_H

#include "_settings.h"  // Settings for what to include etc
#include "sensor/float.h"

class Sensor_Ultrasonic : public Sensor_Float {
  public:
    Sensor_Ultrasonic(const char* const id, const char * const name, float max, const char* color,
      const bool retain, float offset = 0.0, float scale = 1.0);
  protected:
    float offset;
    float scale;
    float convert(float v) override;
    void dispatch(System_Message &msg) override;
    void captiveLines(AsyncResponseStream* response) override;
};
#endif // SENSOR_ULTRASONIC_H
