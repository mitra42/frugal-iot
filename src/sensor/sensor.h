#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "system/base.h"
#include "system/io.h"
#include <vector>

// The UX range for one output - min/max/color, as generated into defaults.h from the server's
// modules.yaml. A base class shared by several chips (Sensor_AHT, Sensor_BMx280) cannot name
// its subclass's macros - DEFAULT_aht21_temperature_min vs DEFAULT_aht20_temperature_min - so
// the subclass passes them down as one of these instead.
struct OutputRange {
  const float min;
  const float max;
  const char* const color;
};

class Sensor : public System_SensorActuator {
  public:
    //Sensor();
    Sensor(const char* id, const char* const name, bool retain);
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
    void captiveLines(AsyncResponseStream* response) override; // Read-only list of the outputs
    String captiveValueLines(); // The "<br>Name: value unit" fragment, for overrides to extend
}; // Class Sensor


#endif // SENSOR_H