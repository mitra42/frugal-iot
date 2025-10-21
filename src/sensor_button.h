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
 
// Note this is a subclass of System_Base, not of Sensor 
 class Sensor_Button : public System_Base {
    public:
      Sensor_Button( const char * const id, const char * const name, const uint8_t pin, const char * const color);
    protected: // Its ok to "unprotect" these if useful
      OUTuint16* output; // TODO convert to an enum 
      void setup() override;
      void loop() override;
      void clickHandlerInner(const clickType type);
      static void clickHandler(const Button2& btn);
      static void longClickHandler(const Button2& btn);
      static void doubleClickHandler(const Button2& btn);
      static void tripleClickHandler(const Button2& btn);
      static Sensor_Button* handler(const Button2& button);
    private:
      Button2* button;
      uint8_t pin;
 };
 
 #endif // SENSOR_BUTTON_H
 