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
 #include "sensor_uint16.h"

class Sensor_Analog : public Sensor_Uint16 {
  public: 
    uint8_t pin;
    
    //Sensor_Analog(const uint8_t p);
    Sensor_Analog(const char* const id, const char * const name, const uint8_t pin, const uint8_t smooth, const uint16_t min, const uint16_t max, const char* color, bool retain);
    virtual void setup() override;
    uint16_t read() override;
}; // Class Sensor_Analog
#endif // SENSOR_ANALOG_H
