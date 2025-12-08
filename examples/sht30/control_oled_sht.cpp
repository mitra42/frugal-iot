#include "control_oled_sht.h"
#include "Frugal-IoT.h"

#ifdef SYSTEM_OLED_WANT // Only compile if have an OLED

Control_Oled_SHT::Control_Oled_SHT(const char* name)
  :
  temperature(new INfloat("control_oled_sht", "temperature", "Temperature", 0, 1, 0, 50, "#ff0000", true)),
  humidity(new INfloat("control_oled_sht", "humidity", "Humidity", 0, 1, 0, 100, "#0000ff", true)),
  battery(new INfloat("control_oled_sht", "battery", "Battery", 0, 0, 0, 5000, "#00ff00", true)),
  //Control_Oled("control_oled_sht", name, { })
  Control_Oled("control_oled_sht", name, std::vector<IN*> { })
  {
      inputs.push_back(temperature);
      inputs.push_back(humidity);
      inputs.push_back(battery);
  }

  void Control_Oled_SHT::act() {
    // Called when any of the inputs change
    Adafruit_SSD1306* display  = &frugal_iot.oled->display;
    display->clearDisplay();
    display->setCursor(0,0);
    display->setTextSize(3);
    //TODO-149 just comes up black despite good value of color565 return
    //Serial.print("Color: "); Serial.print(temperature->color); Serial.println(color565(temperature->color),HEX); 
    //display->setTextColor(color565(temperature->color));
    //display->setTextColor(0xF800, 0x001F); // ALso doesnt work, just prints strange characters
    display->print(temperature->floatValue(),temperature->width);
    display->print("C");
    display->setCursor(0,25);
    //display->setTextColor(color565(humidity->color));
    display->print(humidity->floatValue(), humidity->width);
    display->print("%");    

    display->setCursor(0,50);
    display->setTextSize(1);
    display->print(battery->floatValue(), battery->width);
    display->print("mV");

    display->display();   
  }
  #endif // SYSTEM_OLED_WANT
