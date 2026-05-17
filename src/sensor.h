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
    void setFreshnessMs(uint32_t ms);
  protected:
    std::vector<OUT*> outputs; // Vector of outputs
    const bool retain = false;
    const int qos = 0; // Default to no guarrantee of delivery
    const uint8_t power3v3;
    const uint8_t power0v;
    
    virtual void powerUp();   // Optional power management - override in derived classes
    virtual void powerDown(); // Optional power management - override in derived classes
    
    virtual bool isConnected() { return true; }
    bool connected = true;
    uint32_t freshnessMs_ = 0;
    uint32_t lastReadMs_ = 0;
    OUTbool* connectedOutput = nullptr;
    OUTbool* freshOutput = nullptr;
    
    void markFresh();
    bool isFresh() const;
    virtual void readValidateConvertSet();
    void periodically() override;
    void setup() override;
    void loop() override;
    void infrequently() override;
    void discover() override;
    void dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet);
}; // Class Sensor


#endif // SENSOR_H