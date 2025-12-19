// INA219 Current/Voltage/Power Monitor Sensor
// Adafruit INA219 Guide: https://learn.adafruit.com/adafruit-ina219-current-sensor-breakout
// INA219 Datasheet: https://cdn-shop.adafruit.com/datasheets/ina219.pdf
// INA219 Library: https://github.com/adafruit/Adafruit_INA219

#ifndef SENSOR_INA219_H
#define SENSOR_INA219_H

#include "sensor.h"
#include <Adafruit_INA219.h>

/**
 * INA219 Current/Voltage/Power Monitor Sensor
 * 
 * Provides three outputs:
 * - Voltage (V): Bus voltage reading
 * - Current (mA): Current draw measurement
 * - Battery (%): Calculated battery percentage based on voltage range
 * 
 * Default I2C address: 0x40
 */
class Sensor_INA219 : public Sensor {
public:
    /**
     * Constructor
     * @param id     Sensor identifier
     * @param name   Display name
     * @param maxV   Maximum voltage for percentage calculation (default: 4.2V for LiPo)
     * @param minV   Minimum voltage for percentage calculation (default: 3.0V for LiPo)
     * @param retain Whether to retain MQTT messages
     */
    Sensor_INA219(const char* id, const char* name, float maxV = 4.2, float minV = 3.0, bool retain = true);

protected:
    void setup() override;
    void readValidateConvertSet() override;

private:
    Adafruit_INA219 _ina219;
    float _maxVoltage;
    float _minVoltage;
    bool _initialized;
    
    // Output channels
    OUTfloat* _out_voltage;
    OUTfloat* _out_current;
    OUTfloat* _out_percentage;
};

#endif // SENSOR_INA219_H
