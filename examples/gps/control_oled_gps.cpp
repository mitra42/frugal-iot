#include "control_oled_gps.h"
#include "Frugal-IoT.h"

#ifdef SYSTEM_OLED_WANT

Control_Oled_GPS::Control_Oled_GPS(const char* name)
  : latitude(  new INfloat("control_oled_gps", "latitude",   "Latitude",   0, 6, -90.0f,  90.0f,  "blue",   true)),
    longitude(  new INfloat("control_oled_gps", "longitude",  "Longitude",  0, 6, -180.0f, 180.0f, "blue",   true)),
    altitude(   new INfloat("control_oled_gps", "altitude",   "Altitude",   0, 1, -500.0f, 9000.0f,"green",  true)),
    speed(      new INfloat("control_oled_gps", "speed",      "Speed",      0, 1,    0.0f,  999.0f, "orange", true)),
    course(     new INfloat("control_oled_gps", "course",     "Course",     0, 1,    0.0f,  360.0f, "black",  true)),
    satellites( new INfloat("control_oled_gps", "satellites", "Satellites", 0, 0,    0.0f,   32.0f, "black",  true)),
    hdop(       new INfloat("control_oled_gps", "hdop",       "HDOP",       0, 2,    0.0f,   50.0f, "black",  true)),
    Control_Oled("control_oled_gps", name, std::vector<IN*> { })
{
  inputs.push_back(latitude);
  inputs.push_back(longitude);
  inputs.push_back(altitude);
  inputs.push_back(speed);
  inputs.push_back(course);
  inputs.push_back(satellites);
  inputs.push_back(hdop);
}

void Control_Oled_GPS::setup() {
  Control_Oled::setup();
  if (enabled) {
    Adafruit_SSD1306* display = &frugal_iot.oled->display;
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 28);
    display->print(name);
    display->display();
  }
}

void Control_Oled_GPS::act() {
  if (enabled) {
    Adafruit_SSD1306* display = &frugal_iot.oled->display;
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);

    // Line 1 (y=0): latitude with sign, 6 decimal places
    display->setCursor(0, 0);
    display->print(F("Lat:"));
    display->print(latitude->floatValue(), latitude->width);

    // Line 2 (y=9): longitude with sign, 6 decimal places
    display->setCursor(0, 9);
    display->print(F("Lon:"));
    display->print(longitude->floatValue(), longitude->width);

    // Line 3 (y=18): altitude and course on one line
    display->setCursor(0, 18);
    display->print(F("Alt:"));
    display->print(altitude->floatValue(), altitude->width);
    display->print(F("m Crs:"));
    display->print(course->floatValue(), course->width);

    // Line 4 (y=27): speed
    display->setCursor(0, 27);
    display->print(F("Spd:"));
    display->print(speed->floatValue(), speed->width);
    display->print(F("km/h"));

    // Line 5 (y=36): satellite count and HDOP (signal quality)
    display->setCursor(0, 36);
    display->print(F("Sat:"));
    display->print((int)satellites->floatValue());
    display->print(F(" HDOP:"));
    display->print(hdop->floatValue(), hdop->width);

    display->display();
  }
}

#endif // SYSTEM_OLED_WANT
