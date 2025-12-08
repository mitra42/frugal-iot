// DS18B20 waterproof temperature sensor for soil or environment monitoring
// Supports multiple sensors on a single OneWire bus using sensor index.

#ifndef SENSOR_DS18B20_H
#define SENSOR_DS18B20_H

#include "sensor_float.h"
#include <OneWire.h>
#include <DallasTemperature.h>

class Sensor_DS18B20 : public Sensor_Float {
public:
    /**
     * @brief Constructor for DS18B20 temperature sensor
     * 
     * @param id        Unique ID for the sensor
     * @param name      Human-readable sensor name
     * @param pin       GPIO pin connected to the DS18B20 data line
     * @param index     Sensor index on the OneWire bus (default = 0)
     * @param retain    Whether to retain last sensor value (true/false)
     * 
     * Note: Index 0 reads the first DS18B20 detected. Use higher index values
     * if multiple sensors share the same OneWire bus.
     */
    Sensor_DS18B20(const char* id, const char* name, uint8_t pin, uint8_t index, bool retain);

protected:
    /**
     * @brief Reads the current temperature in Celsius
     * @return float Temperature value or NAN if disconnected
     */
    float readFloat() override;

    /**
     * @brief Initializes the sensor bus and prepares communication
     */
    void setup() override;

private:
    OneWire _oneWire;          // OneWire interface for DS18B20 communication
    DallasTemperature _sensors;// DallasTemperature driver instance
    uint8_t _index;            // Sensor index on the OneWire bus
};

#endif // SENSOR_DS18B20_H
