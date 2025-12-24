#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "system_base.h"
#include <vector>

class Sensor : public System_Base {
  public:
    //Sensor();
    Sensor(const char* id, const char* const name, bool retain);
  protected:
    std::vector<OUT*> outputs; // Vector of outputs
    const bool retain = false;
    const int qos = 0; // Default to no guarrantee of delivery
    unsigned long nextLoopTime = 0;
    
    virtual void readValidateConvertSet();
    void periodically() override;
    void setup() override;
    void discover() override;
    void dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet);
}; // Class Sensor


#endif // SENSOR_H