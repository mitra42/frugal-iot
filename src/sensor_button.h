#ifndef SENSOR_BUTTON_WANT
#define SENSOR_BUTTON_WANT
/* 
 * Frugal IoT button handler 
 * 
 * Detect a button click, and send a message depending on SINGLE, LONG, DOUBLE, or TRIPLE
 */

 #ifdef SENSOR_BUTTON_WANT

 #include <vector>
 #include "Button2.h" // https://github.com/LennartHennigs/Button2
 #include "sensor.h"

 #ifndef SENSOR_BUTTON_MS
  #define SENSOR_BUTTON_MS 10
#endif
#ifndef SENSOR_BUTTON_TOPIC
  #define SENSOR_BUTTON_TOPIC "button"
#endif
#ifndef SENSOR_BUTTON_PIN
  #ifdef LILYGOHIGROW
    #define SENSOR_BUTTON_PIN 35
  #else
    #error No default button defined for your board please define SENSOR_BUTTON_PIN in _local.h
  #endif
#endif
 
 class Sensor_Button : public Sensor {
    public:
      Sensor_Button(uint8_t pin, const char* topicLeaf);
      void setup();
      void loop();
      void clickHandlerInner(clickType type);
      static void clickHandler(Button2& btn);
      static void longClickHandler(Button2& btn);
      static void doubleClickHandler(Button2& btn);
      static void tripleClickHandler(Button2& btn);
      static void newSensor_Button(uint8_t pin, const char* topicLeaf);
      static Sensor_Button* handler(Button2& button);
    private:
      Button2* button;
      uint8_t pin;
 };
 
 extern std::vector<Sensor_Button*> buttons;
 
 #endif // SYSTEM_BUTTON_WANT
 #endif // SENSOR_BUTTON_H
 