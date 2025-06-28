#ifndef SENSOR_BUTTON_H
#define SENSOR_BUTTON_H
/* 
 * Frugal IoT button handler 
 * 
 * Detect a button click, and send a message depending on SINGLE, LONG, DOUBLE, or TRIPLE
 */

 #include <vector>
 #include "Button2.h" // https://github.com/LennartHennigs/Button2
 #include "sensor.h"
 #include "system_frugal.h"

 #ifndef SENSOR_BUTTON_MS
  #define SENSOR_BUTTON_MS 10
#endif
 
 class Sensor_Button : public Sensor {
    public:
      Sensor_Button( const char * const id, const char * const name, uint8_t pin, const char * const color);
      OUTuint16* output; // TODO convert to an enum 
      void setup();
      void frequently();
      void clickHandlerInner(clickType type);
      static void clickHandler(Button2& btn);
      static void longClickHandler(Button2& btn);
      static void doubleClickHandler(Button2& btn);
      static void tripleClickHandler(Button2& btn);
      static Sensor_Button* handler(Button2& button);
    private:
      Button2* button;
      uint8_t pin;
 };
 
 #endif // SENSOR_BUTTON_H
 