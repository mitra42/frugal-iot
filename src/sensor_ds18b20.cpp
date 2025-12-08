#include "sensor_ds18b20.h"

/**
 * @brief Constructor
 * 
 * Sets up OneWire communication and defines operational parameters.
 * Temperature range: -55°C to +125°C
 * Color label "orange" for UI or visualization systems.
 */
Sensor_DS18B20::Sensor_DS18B20(const char* id, const char* name, uint8_t pin, uint8_t index, bool retain)
    : Sensor_Float(id, name, 2, -55, 125, "orange", retain),
      _oneWire(pin),
      _sensors(&_oneWire),
      _index(index) {}

/**
 * @brief Initializes DS18B20 sensor bus
 * 
 * Starts the DallasTemperature library, which scans for devices on the bus.
 * Each connected DS18B20 can be accessed by index or by unique 64-bit address.
 */
void Sensor_DS18B20::setup() {
    _sensors.begin();
    #ifdef SENSOR_DS18B20_DEBUG
        Serial.print(F("DS18B20 sensor initialized on index ")); Serial.println(_index);
    #endif
}

/**
 * @brief Reads temperature from the specified DS18B20 sensor
 * 
 * Requests temperature data from all devices on the OneWire bus and then
 * retrieves the temperature for the configured sensor index.
 * 
 * @return float Temperature in Celsius, or NAN if sensor is disconnected.
 */
float Sensor_DS18B20::readFloat() {
    _sensors.requestTemperatures();                // Trigger measurement on all sensors
    float tempC = _sensors.getTempCByIndex(_index); // Read temperature for specific index

    if (tempC != DEVICE_DISCONNECTED_C) {
        return round (tempC); // Return valid temperature rounded to nearest integer
    } else {
        #ifdef SENSOR_DS18B20_DEBUG
            Serial.print(F("Error: DS18B20 sensor index "));
            Serial.print(_index);
            Serial.println(F(" not detected or disconnected"));
        #endif
        return NAN; // Return Not-A-Number if sensor missing
    }
}
