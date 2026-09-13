/* The OSPIT status display - a port of display.lua onto Control_Oled.
 *
 * OSPIT shows three pages and advances one every time the display timer fires. Here the three
 * pages are three Control_Oled subclasses in a Control_Carousel, which is the library's existing
 * way of doing exactly that, and means the pages can also be selected by hand (a button, or
 * publishing to carousel/select) rather than only cycling.
 *
 *   Page 1  Control_Oled_OspitPower    battery
 *   Page 2  Control_Oled_OspitSoil     the sector moistures and the tank level
 *   Page 3  Control_Oled_OspitNet      IP, WiFi, MQTT
 *
 * Page 1 is battery only for now. OSPIT's version of it is mostly MPPT - solar open-circuit
 * voltage, tracking voltage, charge state, battery temperature - and none of that exists until
 * P4/P5. This is the page that grows then.
 *
 * Pages 2 and 3 follow display.lua closely, with two differences worth knowing:
 *   - a sector with no reading prints "--" rather than OSPIT's "-127". Same meaning, less
 *     arithmetic for a person standing in a field.
 *   - the tank prints "--" when no sender is fitted, which OSPIT cannot distinguish from empty.
 *
 * The display is the I2C SSD1306 on pins 21/22, which is what OSPIT's init.lua loads. It does NOT
 * conflict with the RS485 probes; only the SSD1327 SPI panel does, because that needs pins 16/17.
 */

#ifndef CONTROL_OLED_OSPIT_H
#define CONTROL_OLED_OSPIT_H

#include "_settings.h"

#ifdef ACTUATOR_OLED_WANT

#include "control/oled.h"
#include "control/carousel.h"

// Page 1 - battery. Grows into the MPPT page at P4/P5.
class Control_Oled_OspitPower : public Control_Oled {
  public:
    INfloat* battery;
    Control_Oled_OspitPower();
    void act() override;
};

// Page 2 - what the irrigation actually runs on: one line per sector, plus the tank.
class Control_Oled_OspitSoil : public Control_Oled {
  public:
    // Three, matching the three sectors. A node with a different number wants a different page -
    // this is an example, and the point is that it is short enough to edit.
    INfloat* moisture1;
    INfloat* moisture2;
    INfloat* moisture3;
    INfloat* tank;
    INuint16* active; // Which sector is watering right now, 0 for none. OSPIT has no equivalent
    Control_Oled_OspitSoil();
    void act() override;
};

// Page 3 - connectivity, as display.lua's third page
class Control_Oled_OspitNet : public Control_Oled {
  public:
    Control_Oled_OspitNet();
    void act() override;
};

/* Build all three, add them to frugal_iot.controls and to a carousel, and return the carousel.
 *
 * Only the selected page is `enabled`, which is what stops three controls fighting over one
 * screen - Control_Carousel::act() flips that as the selection moves.
 */
Control_Carousel* ospitDisplay();

#endif // ACTUATOR_OLED_WANT
#endif // CONTROL_OLED_OSPIT_H
