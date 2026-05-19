#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "system_base.h"
#include <vector>

class Sensor : public System_Base {
  public:
    Sensor(const char* id, const char* const name, bool retain, uint8_t power3v3_pin = 0xFF, uint8_t power0v_pin = 0xFF);
    virtual void prepare();   // Calls powerDown for power cycling before sensor read
    virtual void recover();   // Calls powerUp for power cycling after sleep
    void setFreshnessMs(uint32_t ms); // Set freshnessMs config
  protected:
    std::vector<OUT*> outputs; // Vector of outputs
    const bool retain = false;
    const int qos = 0; // Default to no guarrantee of delivery
    const uint8_t power3v3;
    const uint8_t power0v;
    
    virtual void powerUp();   // Optional power management - override in derived classes
    virtual void powerDown(); // Optional power management - override in derived classes
    
    virtual bool isConnected() { return true; } // Can be overridden by sensors to report if they are connected
    uint32_t freshnessMs_ = 0; // Length of time that if haven't had a read its not fresh
    uint32_t lastReadMs_ = 0;  // Last time a change was read from a sensor
    OUTbool* connectedOutput = nullptr; // Reports true if sensor connected
    OUTbool* freshOutput = nullptr;     // Reports true if have a sensor reading within freshnessMs milliseconds
    
    void markFresh(); // Set the time that last change made
    bool isFresh() const; // Returns true if a change detected more recently than freshnessMs
    virtual void readValidateConvertSet();
    void periodically() override;
    void setup() override;
    void discover() override;
    void dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet);
}; // Class Sensor


#endif // SENSOR_H
