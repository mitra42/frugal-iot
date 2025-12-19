// WARNING - THIS COMPILES BUT IS UNTESTED 

// INA219 Current/Voltage/Power Monitor Sensor
// Adafruit INA219 Guide: https://learn.adafruit.com/adafruit-ina219-current-sensor-breakout
// INA219 Datasheet: https://cdn-shop.adafruit.com/datasheets/ina219.pdf
// INA219 Library: https://github.com/adafruit/Adafruit_INA219

#include "_settings.h"
#include "Frugal-IoT.h"
#include "sensor_ina219.h"

/**
 * @brief Constructor for INA219 sensor
 * 
 * Creates three output channels:
 * - Voltage: 0-32V range, yellow color
 * - Current: -3200mA to +3200mA range, orange color
 * - Percentage: 0-100% battery level, green color
 */
Sensor_INA219::Sensor_INA219(const char* id, const char* name, float maxV, float minV, bool retain)
    : Sensor(id, name, retain), 
      _maxVoltage(maxV), 
      _minVoltage(minV), 
      _initialized(false) {
    
    // Create three output channels for this sensor
    // Format: OUTfloat(sensorId, id, name, value, decimals, min, max, color, wireable)
    _out_voltage = new OUTfloat(id, "voltage", "Voltage", 0.0, 2, 0.0, 32.0, "yellow", true);
    outputs.push_back(_out_voltage);
    
    _out_current = new OUTfloat(id, "current", "Current", 0.0, 1, -3200.0, 3200.0, "orange", true);
    outputs.push_back(_out_current);
    
    _out_percentage = new OUTfloat(id, "percentage", "Battery", 0.0, 0, 0.0, 100.0, "green", true);
    outputs.push_back(_out_percentage);
}

/**
 * @brief Initialize the INA219 sensor
 * 
 * Attempts to communicate with the INA219 over I2C.
 * Uses default 32V, 2A calibration range.
 */
void Sensor_INA219::setup() {
    if (!_ina219.begin()) {
        Serial.println(F("INA219: Failed to find chip"));
        _initialized = false;
    } else {
        Serial.println(F("INA219: Initialized successfully"));
        _initialized = true;
        // Default calibration for 32V, 2A range
        // Alternative calibrations:
        // _ina219.setCalibration_32V_1A();
        // _ina219.setCalibration_16V_400mA();
    }
}

/**
 * @brief Read sensor values and update outputs
 * 
 * Reads voltage and current from the INA219 sensor,
 * calculates battery percentage based on voltage range,
 * and updates all three output channels.
 */
void Sensor_INA219::readValidateConvertSet() {
    if (!_initialized) {
        return;
    }

    // Read voltage from INA219
    float voltage = _ina219.getBusVoltage_V();
    _out_voltage->set(voltage);

    // Read current from INA219
    float current = _ina219.getCurrent_mA();
    _out_current->set(current);

    // Calculate battery percentage (linear approximation between min and max voltage)
    float percentage = ((_minVoltage > 0) && (_maxVoltage > _minVoltage)) 
        ? ((voltage - _minVoltage) / (_maxVoltage - _minVoltage)) * 100.0
        : 0.0;
    percentage = constrain(percentage, 0.0, 100.0);
    _out_percentage->set(percentage);

    #ifdef SENSOR_INA219_DEBUG
        Serial.print(F("INA219 - Voltage: "));
        Serial.print(voltage);
        Serial.print(F("V, Current: "));
        Serial.print(current);
        Serial.print(F("mA, Battery: "));
        Serial.print(percentage);
        Serial.println(F("%"));
    #endif
}
