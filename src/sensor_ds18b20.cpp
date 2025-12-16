#include "sensor_ds18b20.h"
#include <cmath>

/**
 * @brief Constructor
 * 
 * Sets up OneWire communication and defines operational parameters.
 * Temperature range: -55°C to +125°C
 * Color label "orange" for UI or visualization systems.
 */
Sensor_DS18B20::Sensor_DS18B20(const char* id, const char* name, uint8_t pin, uint8_t index, bool retain)
    : Sensor_Float(id, name, 1, -55, 125, "orange", retain),  // width=1 for 1 decimal place
      _oneWire(pin),
      _sensors(&_oneWire),
      _index(index) {}

/**
 * @brief Initializes DS18B20 sensor bus
 * 
 * Starts the DallasTemperature library, which scans for devices on the bus.
 * Each connected DS18B20 can be accessed by index or by unique 64-bit address.
 * Sets resolution to 12-bit for full precision (0.0625°C).
 */
void Sensor_DS18B20::setup() {
    _sensors.begin();
    // Set 12-bit resolution for full precision (0.0625°C)
    _sensors.setResolution(12);
    #ifdef SENSOR_DS18B20_DEBUG
        Serial.print(F("DS18B20 sensor initialized on index ")); 
        Serial.print(_index);
        Serial.print(F(" with resolution: "));
        Serial.println(_sensors.getResolution());
    #endif
}

/**
 * @brief Validates the temperature reading
 * 
 * The DS18B20 sensor returns 85°C as its power-on reset value, which indicates
 * an error or uninitialized state. This override rejects:
 * - NaN values (disconnected sensor)
 * - Values >= 80°C (likely power-on reset or error values)
 * 
 * @param v The temperature value to validate
 * @return bool True if the value is valid, false otherwise
 */
bool Sensor_DS18B20::validate(float v) {
    return !std::isnan(v) && (v < 80);
}

/**
 * @brief Reads temperature from the specified DS18B20 sensor with full precision
 * 
 * Requests temperature data from all devices on the OneWire bus and then
 * retrieves the temperature for the configured sensor index.
 * Returns the full precision value WITHOUT rounding.
 * 
 * @return float Temperature in Celsius with decimal precision, or NAN if sensor is disconnected.
 */
float Sensor_DS18B20::readFloat() {
    _sensors.requestTemperatures();                 // Trigger measurement on all sensors
    float tempC = _sensors.getTempCByIndex(_index); // Read temperature for specific index

    if (tempC != DEVICE_DISCONNECTED_C) {
        return tempC;  // Return full precision temperature (no rounding!)
    } else {
        #ifdef SENSOR_DS18B20_DEBUG
            Serial.print(F("Error: DS18B20 sensor index "));
            Serial.print(_index);
            Serial.println(F(" not detected or disconnected"));
        #endif
        return NAN; // Return Not-A-Number if sensor missing
    }
}
