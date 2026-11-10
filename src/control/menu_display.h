// Deep Sleep issues: none in themselves - redrawn from the Control_Menu, which is home after a wake.
#ifndef CONTROL_MENU_DISPLAY_H
#define CONTROL_MENU_DISPLAY_H
/* Frugal IoT - where a Control_Menu is shown. See control/menu.h for the menu itself.
 *
 * Menu_Display_LCD   a character LCD, by sending its text to Actuator_LCD's message input
 * Menu_Display_OLED  the node's OLED, drawn directly - only with ACTUATOR_OLED_WANT
 * Menu_Display_Captive  the captive portal: the screen, refreshed in place, and four buttons
 *
 * Both show the same thing while the menu is home (not in use): either the screen is handed back
 * to another control, e.g. a Control_Oled_Single showing the weight, or the top menu is listed
 * without a cursor, with its live values - so a scale shows "Weight 512 g" when idle.
 */

#include "_settings.h"
#include "control/menu.h"

/* Sends the menu as text, one line per display row separated by "\n", on its output menu/lcd,
 * which is wired to an Actuator_LCD's message input. Line 1 is the menu's title, the rest its rows
 * with ">" at the current one; while editing a number, its name then its value.
 *
 * By default it is wired to this node's own LCD, topicTwig "lcd/message". For one on another
 * node, pass nullptr and wire out to it - in the sketch, or later from the UX:
 *   lcd->out->wireTo("dev/developers/esp32-12bc34/set/lcd/message");
 * Only sent to the LCD - locally, or over MQTT not retained - not published on menu/lcd itself.
 */
// The same defaults as actuator/lcd.h - here too, as the LCD may be on another node, with no Actuator_LCD here
#ifndef ACTUATOR_LCD_COLS
  #define ACTUATOR_LCD_COLS 16
#endif
#ifndef ACTUATOR_LCD_ROWS
  #define ACTUATOR_LCD_ROWS 2
#endif

class Menu_Display_LCD : public Menu_Display {
  public:
    Menu_Display_LCD(uint8_t cols = ACTUATOR_LCD_COLS, uint8_t rows = ACTUATOR_LCD_ROWS, const char* topicTwig = "lcd/message");
    OUTtext* out;
    OUT* output() override { return out; }
    void render(Control_Menu* menu) override;
  protected:
    const uint8_t cols;
    const uint8_t rows;
    String last; // Not resent when nothing changed - an LCD clears and redraws on every message, which flickers
};

#ifdef ACTUATOR_OLED_WANT
/* Draws the menu on frugal_iot.oled: the title in an inverted bar, then as many rows as fit,
 * scrolling to keep the current one in view, with a triangle at it and each row's value at the
 * right. While editing a number, its name, the value large, and the range.
 *
 * home is the control that owns the screen while the menu is not in use - it is disabled while
 * the menu is shown and redrawn when it goes home. Without one, home lists the top menu.
 */
class Menu_Display_OLED : public Menu_Display {
  public:
    explicit Menu_Display_OLED(Control* home = nullptr);
    void render(Control_Menu* menu) override;
  protected:
    Control* home;
    void drawMenu(Control_Menu* menu, bool cursor);
    void drawEdit(Control_Menu* menu);
};
#endif // ACTUATOR_OLED_WANT

#ifndef CONTROL_MENU_CAPTIVE_REFRESH_MS
  #define CONTROL_MENU_CAPTIVE_REFRESH_MS 1000 // How often the portal page asks for the screen
#endif

/* The menu on the captive portal page: the screen as the other displays show it, then up, down,
 * back and accept buttons in a diamond. The buttons send menu/up etc, exactly as a physical
 * button does, so the portal and the buttons drive the same menu and each sees what the other did.
 * The page is not reloaded - it asks for the screen every CONTROL_MENU_CAPTIVE_REFRESH_MS, and
 * straight after a button. Unlike the others it draws nothing on render(); the page pulls.
 */
class Menu_Display_Captive : public Menu_Display {
  public:
    void render(Control_Menu* menu) override { }
    unsigned long captiveRefreshMs() override { return CONTROL_MENU_CAPTIVE_REFRESH_MS; }
    void captiveLines(Control_Menu* menu, AsyncResponseStream* response) override;
    void captiveValues(Control_Menu* menu, Captive_Values* values) override;
  protected:
    String screen(Control_Menu* menu); // The HTML for what is shown now
};

#endif // CONTROL_MENU_DISPLAY_H
