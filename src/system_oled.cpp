/* Frugal IoT - OLED Display hanler
 * This is a port of code from demo for TTGO Lora board - expand as needed
 * 
 * See https://github.com/mitra42/frugal-iot/issues/149 
 */
#include "_settings.h"
#ifdef SYSTEM_OLED_WANT
#include "system_oled.h"
//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//OLED pins

System_OLED::System_OLED() : Frugal_Base() {
    // Constructor code here, if needed
    Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
}
System_OLED::setup() : Frugal_Base::setup() {
    // Setup code here, if needed
}



#endif // SYSTEM_OLED_WANT