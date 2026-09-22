// Deep Sleep issues: none - the tare offset and calibration scale are persisted.
/*
 * Sensor Analog
 * Read from a pin and send message
 *
 * See https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html for lots more on ESP ADCs
 *
 * Configuration options.
 * Optional: SENSOR_ANALOG_REFERENCE for ESP8266 only  // TODO-141 phase out
 * Optional: SENSOR_ANALOG_ATTENTUATION // TODO-141 phase out
 * TODO: There is a lot more clever stuff on https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html
 * Its ESP32 specific, but looks like a range of capabilities that could be integrated.
 */

#ifndef SENSOR_ANALOG_H
#define SENSOR_ANALOG_H

 #include "sensor/sensor.h"
 #include "sensor/float.h"

/* Raw ADC, reported after a linear offset-and-scale.
 *
 * readInt() is analogRead(), i.e. RAW COUNTS with the ESP32 ADC's nonlinearity left in - the
 * error is S-shaped and worst near the ends of the range. Nothing here corrects for it, because
 * a soil probe or a tank sender is calibrated against its own two end points anyway (see tare()
 * and calibrate()), which absorbs a good deal of it.
 *
 * Sensor_Voltage is the exception: it overrides readInt() with analogReadMilliVolts(), which
 * applies the chip's factory calibration. If you are porting code that carries its own
 * correction curve, read the note in voltage.h before copying the constants - they may be
 * correcting something that has already been corrected.
 *
 * convert() is virtual, so a subclass whose transfer function is not a straight line can replace
 * it outright rather than trying to express the curve as an offset and a scale.
 */
class Sensor_Analog : public Sensor_Float {
  public:
    Sensor_Analog(const char* const id, const char * const name, const uint8_t pin, const uint8_t width, const float min, const float max, int offset, float scale, const char* color, bool retain);
    float readValidateConvert() override;
  protected:
    uint8_t pin;    
    int offset;
    float scale;
    //Sensor_Analog(const uint8_t p);
    virtual void setup() override;
    virtual int readInt(); // Not overriding - its different return
    virtual bool validate(int v);
    virtual float convert(int v);
    void tare();
    void calibrate(float v);
    void dispatch(System_Message &msg) override;
}; // Class Sensor_Analog
#endif // SENSOR_ANALOG_H
