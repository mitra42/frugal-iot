#include "control_oled_ht.h"
#include "Frugal-IoT.h"

#ifdef ACTUATOR_OLED_WANT // Only compile if have an OLED

Control_Oled_HT::Control_Oled_HT(const char* name)
  :
  temperature(new INfloat("control_oled_ht", "temperature", "Temperature", 0, 1, 0, 50, "#ff0000", true)),
  humidity(new INfloat("control_oled_ht", "humidity", "Humidity", 0, 1, 0, 100, "#0000ff", true)),
  battery(new INfloat("control_oled_ht", "battery", "Battery", 0, 0, 0, 5000, "#00ff00", true)),
  //Control_Oled("control_oled_ht", name, { })
  Control_Oled("control_oled_ht", name, std::vector<IN*> { })
  {
      inputs.push_back(temperature);
      inputs.push_back(humidity);
      inputs.push_back(battery);
  }

  void Control_Oled_HT::act() {
    if (!enabled) return;
    // Called when any of the inputs change
    Adafruit_SSD1306* display  = &frugal_iot.oled->display;
    #ifdef ACTUATOR_OLED_DEBUG
      Serial.println(F("Writing fresh to oLED"));
    #endif
    display->clearDisplay();
    display->setCursor(0,0);
    #ifdef OLED_IS_HW675
      display->setTextSize(2);
    #else
      display->setTextSize(3);
    #endif

    //TODO-149 just comes up black despite good value of color565 return
    //Serial.print("Color: "); Serial.print(temperature->color); Serial.println(color565(temperature->color),HEX); 
    //display->setTextColor(color565(temperature->color));
    //display->setTextColor(0xF800, 0x001F); // ALso doesnt work, just prints strange characters
    display->print(temperature->floatValue(),temperature->width);
    display->print("C");
    #ifdef OLED_IS_HW675
      display->setCursor(0,20);
    #else
      display->setCursor(0,25);
    #endif
    //display->setTextColor(color565(humidity->color));
    display->print(humidity->floatValue(), humidity->width);
    display->print("%");    
    #ifndef OLED_IS_HW675
      display->setCursor(0,50);
      display->setTextSize(1);
      display->print(battery->floatValue(), battery->width);
      display->print("mV");
    #endif

    display->display();   
  }
  #endif // ACTUATOR_OLED_WANT
