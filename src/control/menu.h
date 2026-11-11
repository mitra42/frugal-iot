// Deep Sleep issues: where the user is in the menu is lost, so a wake starts at home. Use with Power_Loop or Power_Light - a menu needs the node awake to answer the buttons.
#ifndef CONTROL_MENU_H
#define CONTROL_MENU_H
/* Frugal IoT - a local menu UX, in three parts that mix and match
 *
 *  1. WHAT the menu holds - Menu, MenuLine, MenuNumber (below). A tree built in the sketch.
 *  2. HOW it is driven   - Control_Menu and its subclasses, taking up/down/accept/back events.
 *  3. WHERE it is shown  - Menu_Display subclasses, attached to the Control_Menu with addDisplay().
 *
 * So the same tree for, say, a weighing machine can be driven by four buttons or by one, and shown
 * on an LCD, an OLED, the captive portal, or several at once.
 *
 * The control's id is always "menu" - one menu per node - which is also why the items' IOs
 * can be created before the Control_Menu: they are all "menu/<item id>".
 *
 * Inputs: menu/up, menu/down, menu/accept, menu/back. Each message to one of them is a key press,
 * whatever its payload - except that a number in the payload (from Sensor_Button's "held") is how
 * many repeats the key has been held for, which speeds up number editing. Any can be left unwired.
 * menu/value, while a number is being edited, sets it to the payload and accepts it - so a display
 * that can take typing, i.e. the captive portal, need not step there.
 *
 * Wiring, e.g. four buttons:      b->singleClick->wireTo(menu->up); b->held->wireTo(menu->up); ...
 *                 one button:     b->singleClick->wireTo(menu->down); b->doubleClick->wireTo(menu->accept);
 *                                 b->longClick->wireTo(menu->back);
 *
 * Each MenuLine and MenuNumber has two IOs:
 *   menu/<id>       an OUT, published when the item is accepted, and sent to the item's topic
 *   menu/<id>_live  an IN, wire it to the reading to show beside the item, e.g. the current weight
 *
 * Choosing item ids: they share the menu/ topics with the control's own IOs, so within one menu an
 * item must not be called up, down, accept, back or value (the control's inputs and messages), or
 * lcd (Menu_Display_LCD's output, menu/lcd, if one is added) - nor another item's id + "_live".
 * Nothing checks this; a clash makes the item's messages be taken as the control's.
 *
 * Configuration: CONTROL_MENU_DEPTH (8), CONTROL_MENU_TIMEOUT_MS (30000), CONTROL_MENU_NOTICE_MS (2000),
 * CONTROL_MENU_DEBUG
 */

#include "_settings.h"
#include <vector>
#include "control/control.h"
#include "system/language.h"

#ifndef CONTROL_MENU_DEPTH
  #define CONTROL_MENU_DEPTH 8 // How many menus deep the history goes - deeper ones drop the oldest
#endif
#ifndef CONTROL_MENU_NOTICE_MS
  #define CONTROL_MENU_NOTICE_MS 2000 // How long "OK <what was sent>" stays up after an accept
#endif
#ifndef CONTROL_MENU_TIMEOUT_MS
  #define CONTROL_MENU_TIMEOUT_MS 30000 // Return home after this long with no key pressed, 0 = never
#endif

/* Text in several languages. English is required and is the fallback; others are sparse -
 * only the languages that have a translation take any space.
 */
class MenuText {
  public:
    explicit MenuText(const char* en);
    const char* get() const; // In the current language (T), or English
    void add(Language_t lang, const char* text);
    const char* const en;
  protected:
    std::vector<std::pair<Language_t, const char*>> others;
};

enum MenuKind : uint8_t { MENU_MENU, MENU_LINE, MENU_NUMBER };

class Menu;

class MenuItem {
  public:
    const MenuKind kind; // Rather than dynamic_cast, as RTTI is off on ESP8266
    MenuText text;
    MenuItem* translate(Language_t lang, const char* text); // Chainable: (new MenuLine(...))->translate(Language_DE, "...")
    virtual String valueText(); // Shown after the text, e.g. "512 g" - empty if nothing to show
    // Gather this item's IOs (and its children's) for Control_Menu. visited stops a menu that links
    // back up the tree from being walked forever, or an item in two menus being counted twice.
    virtual void collect(std::vector<IN*>& ins, std::vector<OUT*>& outs, std::vector<MenuItem*>& visited);
    virtual ~MenuItem() = default;
  protected:
    MenuItem(MenuKind kind, const char* text);
    bool visit(std::vector<MenuItem*>& visited); // False if already visited
};

// A list of items. Selecting it opens it.
class Menu : public MenuItem {
  public:
    std::vector<MenuItem*> items;
    explicit Menu(const char* text);
    Menu* add(MenuItem* item); // Chainable
    void collect(std::vector<IN*>& ins, std::vector<OUT*>& outs, std::vector<MenuItem*>& visited) override;
};

/* A line that, when accepted, sends a message. What it sends, in order of preference:
 *   payload, if given                       e.g. "0" to loadcell/output, which tares it
 *   the live value, if live is wired        e.g. take the current weight and send it
 *   "1"                                     a plain trigger
 * It is published on menu/<id>, and also sent to topicTwig (on this node) if given - or wire
 * out somewhere else, including another node, with out->wireTo(...).
 */
class MenuLine : public MenuItem {
  public:
    MenuLine(const char* id, const char* text, const char* topicTwig = nullptr, const char* payload = nullptr);
    OUTtext* out;
    INtext* live;
    const char* payload;
    String valueText() override;
    String acceptPayload();
    void collect(std::vector<IN*>& ins, std::vector<OUT*>& outs, std::vector<MenuItem*>& visited) override;
};

/* A number that, when accepted, opens for editing. Up and down step it between min and max,
 * wrapping round at either end; accept sends it if it changed, back leaves it unchanged.
 * Editing starts from the live value - wire live to whatever the number sets, so it starts from
 * what is really there. Without a live wire it starts from the last value accepted (or "value").
 */
class MenuNumber : public MenuItem {
  public:
    MenuNumber(const char* id, const char* text, const char* topicTwig,
      float value, float min, float max, float step = 1, uint8_t decimals = 0);
    OUTtext* out;
    INfloat* live;
    float min;
    float max;
    float step;
    uint8_t decimals;
    bool sendEachChange = false; // True to send on every up/down, rather than only on accept
    String valueText() override;
    String format(float v);
    void collect(std::vector<IN*>& ins, std::vector<OUT*>& outs, std::vector<MenuItem*>& visited) override;
};

class Control_Menu;

// Where the menu is shown. Subclasses decide the layout; they read the state from the Control_Menu.
// The LCD and OLED ones are in control/menu_display.h.
class Menu_Display {
  public:
    // Called after every change, including going home, and when a live value changes - home or not
    virtual void render(Control_Menu* menu) = 0;
    virtual ~Menu_Display() = default;
    // Helpers for character-cell layouts
    static String fit(const String& left, const String& right, uint8_t width); // left, then right right-aligned, cut to width
    static uint8_t firstRow(uint8_t row, uint8_t visible); // First row to show so that row is in view
    static String rowValue(Control_Menu* menu, uint8_t r);  // The item's value, ">" for a submenu, or empty
    // An output the display sends through, if any - addDisplay() makes it one of the menu's, so it is
    // set up, discovered and rewireable like the rest
    virtual OUT* output() { return nullptr; }
    // The captive portal pulls rather than being pushed to - only Menu_Display_Captive uses these
    virtual unsigned long captiveRefreshMs() { return 0; }
    virtual void captiveLines(Control_Menu* menu, AsyncResponseStream* response) { }
    virtual void captiveValues(Control_Menu* menu, Captive_Values* values) { }
};

// Prints the menu to Serial - for trying out a menu with no display attached
class Menu_Display_Serial : public Menu_Display {
  public:
    void render(Control_Menu* menu) override;
  protected:
    bool homeShown = false; // Only say "home" once, not on every reading
};

/* Navigation state, history, and sending. How the keys move around is left to subclasses.
 *
 * The menu is "active" once a key is pressed, and goes back home - inactive - on a timeout or
 * on back from the top. While home, displays should leave the screen to whatever else uses it.
 */
class Control_Menu : public Control {
  public:
    Control_Menu(const char* name, Menu* top);
    INtext* up;
    INtext* down;
    INtext* accept;
    INtext* back;
    Menu* top;
    Menu* menu;                 // The menu currently shown
    uint8_t row = 0;            // 0-based row in menu
    bool active = false;        // False = home, nothing shown
    bool backrow = false;       // True to show "Back" as the last row - set in setup() if back is not wired
    MenuNumber* editing = nullptr; // The number being edited, or nullptr when navigating
    float editValue = 0;
    std::vector<Menu_Display*> displays;
    Control_Menu* addDisplay(Menu_Display* display); // Chainable. Before frugal_iot.setup()
    // For displays
    uint8_t rows();             // Items, plus one if backrow
    bool isBackRow(uint8_t r);
    MenuItem* item(uint8_t r);  // nullptr for the back row
    String rowText(uint8_t r);  // String, as "Back" is a flash string on ESP8266
    const char* title();        // The menu's, or while editing the number's
    // What was just sent, e.g. "Calibrate 512 g" or "Tare" - for CONTROL_MENU_NOTICE_MS after an
    // accept, otherwise empty. So the user can see it was taken.
    String notice;
    String heading();           // "OK " + notice while there is one, otherwise title() - for a display's top line
    void setup() override;
    void loop() override;
    void dispatch(System_Message &msg) override;
    void act() override;        // A live value changed - redraw
    // No sleep while the menu is in use, i.e. until CONTROL_MENU_TIMEOUT_MS after the last key.
    // Note with a timeout of 0 that is until back is pressed from the top menu.
    bool okToSleep(unsigned long ms, uint16_t how) override { return !active; }
    // Passed on to the displays, for Menu_Display_Captive
    void captiveLines(AsyncResponseStream* response) override;
    unsigned long captiveRefreshMs() override;
    void captiveValues(Captive_Values* values, const String& moduleId) override;
  protected:
    struct HistoryEntry { Menu* menu; uint8_t row; };
    HistoryEntry history[CONTROL_MENU_DEPTH];
    uint8_t depth = 0;
    unsigned long lastKey = 0;
    float editStart = 0;        // To tell on accept whether it changed
    unsigned long noticeAt = 0;
    void sent(MenuItem* i, const String& value); // Put up the notice
    // The four keys - count is how many repeats it has been held for, 1 for a click
    virtual void keyUp(uint16_t count) = 0;
    virtual void keyDown(uint16_t count) = 0;
    virtual void keyAccept() = 0;
    virtual void keyBack() = 0;
    // Building blocks for the subclasses
    void open(Menu* m);         // Remember where we are and show m; if m is already in the history, go back to it
    void close();               // Back to the previous menu, or home from the top
    void goHome();
    void sendLine(MenuLine* line);
    void startEdit(MenuNumber* n);
    void sendNumber();          // Send editValue
    void endEdit(bool keep);    // keep=false puts back the value editing started from
    void typed(const String& payload); // menu/value - the number being edited is set to it, and accepted
    void render();
};

/* The first way of driving it - see the comments on each key in menu.cpp.
 * Up/down move through the rows, wrapping round; accept opens a menu, sends a line, or edits a
 * number; back closes. With back not wired there is a "Back" row at the end of each menu instead.
 */
class Control_Menu_Simple : public Control_Menu {
  public:
    Control_Menu_Simple(const char* name, Menu* top);
  protected:
    void keyUp(uint16_t count) override;
    void keyDown(uint16_t count) override;
    void keyAccept() override;
    void keyBack() override;
    void step(int8_t direction, uint16_t count);
};

#endif // CONTROL_MENU_H
