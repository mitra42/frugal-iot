/*  Frugal IoT - Health sensor 
 * 
 *  This will be an aggregation of health fields 
 * 
 * See https://github.com/mitra42/frugal-iot/issues/116 (WiFi strength)
 * Several other issues as well
*/

#ifndef SENSOR_HEALTH_H
#define SENSOR_HEALTH_H
#include "_settings.h"  // Settings for what to include etc

#include "sensor.h"

class Sensor_Health : public Sensor {
  public:
    Sensor_Health(const char* const id, const char* const name);
  protected:
    OUTuint16* wifibars;
    OUTtext* wifissid;
    // TODO-116 more outputs to come here: uptime 
    //void setup() override; 
    void readValidateConvertSet() override; 
  private:
 };
 
#endif // SENSOR_HEALTH_H
