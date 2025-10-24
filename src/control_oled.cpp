/* Frugal IoT - Control OLED
 * 
 * A superclass that will be subclassed to display certain things on the systems OLED
 *  
 * See https://github.com/mitra42/frugal-iot/issues/149
 * 
 * TODO-149 example class in here would be moved to the app's .ino or main.cpp in the example and then this comment deleted 
 * 
*/
#include "_settings.h"
#include "control.h"
#include "control_oled.h"
#include "system_frugal.h"

Control_Oled::Control_Oled(const char* id, const char* name, std::vector<IN*> ii)
  : Control(id, name, ii, std::vector<OUT*> {} ) //std::vector<OUT*> {}
  {}
Control_Oled::Control_Oled(const char* const id, const char* const name)
  : Control_Oled(id, name,  std::vector<IN*> {})
  {}
uint16_t Control_Oled::color565(const char* p1) {
    if (p1[0] == '#') {
    p1 += 1; // Skip # in #x030a1
  } else if (p1[0] == '0' && p1[1] == 'x') {
    p1 += 2; // Skip 0x
  }
  uint32_t rgb = strtoul(p1, nullptr, 16);
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (rgb >> 8) & 0xFF;
  uint8_t b = rgb & 0xFF;
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

Control_Oled_SHT::Control_Oled_SHT(const char* name)
  :
  temperature(new INfloat("control_oled_sht", "temperature", "Temperature", 0, 3, 0, 50, "#ff0000", true)),
  humidity(new INfloat("control_oled_sht", "humidity", "Humidity", 0, 3, 0, 100, "#0000ff", true)),
  Control_Oled("control_oled_sht", name, { temperature, humidity }    
  )
  {}

  void Control_Oled_SHT::act() {
    // Called when any of the inputs change
    Adafruit_SSD1306* display  = &frugal_iot.oled->display;
    display->clearDisplay();
    display->setCursor(0,0);
    display->setTextSize(20);
    display->setTextColor(color565(inputs[0]->color));
    display->print(temperature->floatValue());
    display->print("°C");
    display->setCursor(0,22);
    display->setTextSize(20);
    display->setTextColor(color565(inputs[1]->color));
    display->print(humidity->floatValue());
    display->print("%");    
  }

