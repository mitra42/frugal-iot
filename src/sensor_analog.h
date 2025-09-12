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

 #include "sensor.h"
 #include "sensor_float.h"

class Sensor_Analog : public Sensor_Float {
  public:
    Sensor_Analog(const char* const id, const char * const name, const uint8_t pin, const uint8_t width, const float min, const float max, int offset, float scale, const char* color, bool retain);
  protected:
    uint8_t pin;    
    int offset;
    float scale;
    //Sensor_Analog(const uint8_t p);
    virtual void setup() override;
    virtual int readInt(); // Not overriding - its different return
    virtual bool validate(int v);
    virtual float convert(int v);
    virtual void readValidateConvertSet() override;
    void tare();
    void calibrate(float v);
    void dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet) override;
}; // Class Sensor_Analog
#endif // SENSOR_ANALOG_H
