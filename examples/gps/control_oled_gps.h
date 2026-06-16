#ifndef CONTROL_OLED_GPS_H
#define CONTROL_OLED_GPS_H

#include "control_oled.h"

// Displays GPS fix data on the onboard SSD1306 OLED.
// Wire each input to the matching Sensor_GPS output path in main.cpp.
// utc_time is not shown here because OUTtext→INtext wiring is not yet
// supported by the framework; it is available on the gps/utc_time MQTT topic.
class Control_Oled_GPS : public Control_Oled {
  public:
    INfloat* latitude;
    INfloat* longitude;
    INfloat* altitude;
    INfloat* speed;
    INfloat* course;
    INfloat* satellites;  // receives OUTuint16 value via float conversion
    INfloat* hdop;
    Control_Oled_GPS(const char* name);
    void setup() override;
    void act() override;
};

#endif // CONTROL_OLED_GPS_H
