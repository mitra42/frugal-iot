#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "system_base.h"
#include <vector>

class Sensor : public System_Base {
  public:
    //Sensor();
    Sensor(const char* id, const char* const name, bool retain, uint8_t power3v3_pin = 0xFF, uint8_t power0v_pin = 0xFF);
    virtual void prepare();   // Calls powerDown for power cycling before sensor read
    virtual void recover();   // Calls powerUp for power cycling after sleep
  protected:
    std::vector<OUT*> outputs; // Vector of outputs
    const bool retain = false;
    const int qos = 0; // Default to no guarrantee of delivery
    const uint8_t power3v3;
    const uint8_t power0v;
    
    virtual void powerUp();   // Optional power management - override in derived classes
    virtual void powerDown(); // Optional power management - override in derived classes
    
    virtual void readValidateConvertSet();
    void periodically() override;
    void setup() override;
    void discover() override;
    void dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet);
}; // Class Sensor


#endif // SENSOR_H