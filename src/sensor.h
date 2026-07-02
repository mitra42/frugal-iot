#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "system_base.h"
#include <vector>

class Sensor : public System_SensorActuator {
  public:
    //Sensor();
    Sensor(const char* id, const char* const name, bool retain);
    Sensor(const char* id, const char* const name, bool retain, uint8_t power3v3, uint8_t power0v); // Function missing will delete
    virtual void prepare();   // Calls powerDown for power cycling before sensor read
    virtual void recover();   // Calls powerUp for power cycling after sleep
  protected:
    std::vector<OUT*> outputs; // Vector of outputs
    const bool retain = false;
    const int qos = 0; // Default to no guarrantee of delivery
    
    virtual void readValidateConvertSet();
    void periodically() override;
    void setup() override;
    void discover() override;
    void dispatch(System_Message &msg) override;
}; // Class Sensor


#endif // SENSOR_H