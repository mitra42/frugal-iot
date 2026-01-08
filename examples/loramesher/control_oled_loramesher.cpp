#include "control_oled_loramesher.h"
#include "Frugal-IoT.h"

#ifdef SYSTEM_OLED_WANT // Only compile if have an OLED

Control_Oled_LoRaMesher::Control_Oled_LoRaMesher(const char* name)
  :
  //temperature(new INfloat("control_oled_sht", "temperature", "Temperature", 0, 1, 0, 50, "#ff0000", true)),
  //humidity(new INfloat("control_oled_sht", "humidity", "Humidity", 0, 1, 0, 100, "#0000ff", true)),
  battery(new INfloat("control_oled_loramesher", "battery", "Battery", 0, 0, 0, 5000, "#00ff00", true)),
  //Control_Oled("control_oled_sht", name, { })
  Control_Oled("control_oled_loramesher", name, std::vector<IN*> { })
  {
      //inputs.push_back(temperature);
      //inputs.push_back(humidity);
      inputs.push_back(battery);
  }

  void Control_Oled_LoRaMesher::act() {
    // Called when any of the inputs change
    //TODO-176 probably want other things to trigger that aren't "inputs"
    Adafruit_SSD1306* display  = &frugal_iot.oled->display;
    display->clearDisplay();
    display->setCursor(0,0);
    display->setTextSize(1);
    display->print(frugal_iot.description);
    display->setTextSize(1);
    display->setCursor(0,10);
    display->print(frugal_iot.loramesher->checkRoleString());
    display->setCursor(0,20);
    display->print("last packet:");
    #ifdef SYSTEM_LORAMESHER_DEBUG
      display->setCursor(0,30);
      display->print(frugal_iot.loramesher->lastTopicPath);
      display->setCursor(0,40);
      display->print(frugal_iot.loramesher->lastPayload);      
    #endif
    display->setCursor(0,50);
    display->print(battery->floatValue(), battery->width);
    display->print("mV");
    display->display();   
  }
  #endif // SYSTEM_OLED_WANT
