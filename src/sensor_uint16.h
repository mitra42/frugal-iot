#ifndef SENSOR_UINT16_H
#define SENSOR_UINT16_H

#include <Arduino.h>
#include "system_base.h"
#include "sensor.h"

class Sensor_Uint16 : public Sensor {
  public:
//    uint16_t value = 0;
    OUTuint16* output;
    // If non-zero smoothed by this many bits (newSmoothedValue = oldSmoothedValue - (oldSmoothedValue>>smooth) + (reading))
    // Be careful of overflow - e.g. if 10 bit analog read then max smooth can be is 6 to smooth over 2^6 = 64 readings
    // Note this functionality might get pushed to a superclass
    uint8_t smooth = 0; 

    //Sensor_Uint16();
    Sensor_Uint16(const char* const id, const char * const name, const uint8_t smooth, uint16_t min, uint16_t max, const char* color, const unsigned long ms, bool retain);
    virtual uint16_t read();
    virtual void set(const uint16_t newvalue);
    virtual void readAndSet();
    virtual bool valid(uint16_t newvalue);
    virtual String advertisement();
    void dispatchTwig(const String &topicSensorId, const String &topicLeaf, const String &payload, bool isSet);
};
#endif // SENSOR_UINT16_H
