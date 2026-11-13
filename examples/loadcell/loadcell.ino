/*
 *  Frugal IoT example - Load Cell
 *
 *  A load cell (HX711) scale, with a local menu driven by buttons - to see the weight, tare it,
 *  calibrate it, and choose a language. The menu is in three parts that mix and match - see
 *  control/menu.h:
 *    WHAT  the tree of Menu, MenuLine and MenuNumber built below
 *    HOW   Control_Menu_Simple, driven by up / down / accept / back buttons
 *    WHERE the displays attached with addDisplay() - always Serial, so it runs on a bare board,
 *          plus an LCD with ACTUATOR_LCD_WANT, an LCD on another node with CONTROL_MENU_LCD_NODE,
 *          the OLED on a board with one (ACTUATOR_OLED_WANT), the captive portal with
 *          CONTROL_MENU_CAPTIVE_WANT
 *
 *  Buttons go between the pin and GND, on the pins set per board in platformio.ini. Back is
 *  optional: without it each menu ends in a "Back" row. Or, with SENSOR_BUTTON_ONE_PIN, one button:
 *  click for the next row, double click to accept, hold to step fast, and the "Back" row to go back.
 *
 *  Without SENSOR_LOADCELL_DOUTPIN there is no load cell - the menu works, with nothing to weigh,
 *  for trying out the menu on a board with none attached.
 */

#include "Frugal-IoT.h"

// Change the parameters here to match your ...
// organization, project, id, description
System_Frugal frugal_iot(SYSTEM_FRUGAL_ORG, SYSTEM_FRUGAL_PROJECT, "loadcell", "Load Cell");

// Pins come from platformio.ini, per board - see the build_flags_pins_* groups there
#if defined(SENSOR_LOADCELL_DOUTPIN) != defined(SENSOR_LOADCELL_SCKPIN)
  #error Define both SENSOR_LOADCELL_DOUTPIN and SENSOR_LOADCELL_SCKPIN in platformio.ini, or neither for no load cell
#endif
#if !defined(SENSOR_BUTTON_ONE_PIN) && (!defined(SENSOR_BUTTON_UP_PIN) || !defined(SENSOR_BUTTON_DOWN_PIN) || !defined(SENSOR_BUTTON_ACCEPT_PIN))
  #error Define SENSOR_BUTTON_UP_PIN, _DOWN_PIN and _ACCEPT_PIN (_BACK_PIN is optional), or SENSOR_BUTTON_ONE_PIN, in platformio.ini
#endif
#ifndef SENSOR_LOADCELL_TIMES
  #define SENSOR_LOADCELL_TIMES 9 // How many measurements to take for a reading - it takes the median
#endif
#ifndef SENSOR_LOADCELL_OFFSET
  #define SENSOR_LOADCELL_OFFSET 0 // Just at first start - this will be calibrated
#endif
#ifndef SENSOR_LOADCELL_SCALE
  #define SENSOR_LOADCELL_SCALE 2000 // Just at first start - this will be calibrated
#endif

void setup() {
  // Battery sensor has to come before pre_setup, all others should come after
  #ifdef SENSOR_BATTERY_PIN
    frugal_iot.configure_battery(SENSOR_BATTERY_PIN);
  #endif

  // Configure power handling - type, cycle_ms, wake_ms - see "Power Modes" in the library's docs.
  // A menu needs the node awake to answer its buttons - so Loop (awake all the time), not Deep;
  // a reading every 2 seconds.
  frugal_iot.configure_power(Power_Loop, 2000, 2000);

  frugal_iot.pre_setup(); // Encapsulate setting up and starting serial and read main config
  // The node fetches its OWN broker credential rather than sharing the organization's password.
  // Host and secret both come from platformio.ini; the secret belongs in the uncommitted
  // <name>-local.ini, never in a sketch. Built with no secret, the node is refused and listed on
  // the dashboard's Nodes card for an administrator to approve - that is by design.
  // The shared-password form still works - comment the last line and uncomment this to go back:
  //frugal_iot.configure_mqtt(SYSTEM_MQTT_HOST, SYSTEM_MQTT_USER, SYSTEM_MQTT_PASSWORD);
  frugal_iot.configure_mqtt_enrolled(SYSTEM_MQTT_HOST, SYSTEM_MQTT_ENROL_SECRET);

  // Add local wifis here, or see instructions in the wiki for adding via the /data
  //frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));

  #ifdef SENSOR_LOADCELL_DOUTPIN
    Sensor_LoadCell* loadcell = new Sensor_LoadCell("loadcell", "Load Cell", 100000, DEFAULT_loadcell_loadcell_color, true,
      SENSOR_LOADCELL_DOUTPIN, SENSOR_LOADCELL_SCKPIN, SENSOR_LOADCELL_TIMES, SENSOR_LOADCELL_OFFSET, SENSOR_LOADCELL_SCALE);
    frugal_iot.sensors->add(loadcell);
  #endif

  // ==== WHAT the menu holds ====
  // Shows the weight; accept takes the reading and publishes it on menu/weight
  MenuLine* weight = new MenuLine("weight", "Weight");
  #ifdef SENSOR_LOADCELL_DOUTPIN
    weight->live->wireFrom(loadcell);
  #endif
  weight->live->unit = "g";
  weight->translate(Language_DE, "Gewicht")->translate(Language_ID, "Berat");

  // Sending 0 to the load cell's output tares it - see Sensor_LoadCell::dispatch
  MenuLine* tare = new MenuLine("tare", "Tare", "loadcell/output", "0");
  tare->translate(Language_DE, "Tarieren");

  // Put a known weight on, then set this to it. Editing starts from the current reading.
  MenuNumber* calibrate = new MenuNumber("calibrate", "Calibrate", "loadcell/output", 0, 0, 100000, 1, 0);
  #ifdef SENSOR_LOADCELL_DOUTPIN
    calibrate->live->wireFrom(loadcell);
  #endif
  calibrate->live->unit = "g";
  calibrate->translate(Language_DE, "Kalibrieren")->translate(Language_ID, "Kalibrasi");

  Menu* language = new Menu("Language");
  language->add(new MenuLine("lang_en", "English", "captive/language_code", "EN"))
          ->add(new MenuLine("lang_de", "Deutsch", "captive/language_code", "DE"))
          ->add(new MenuLine("lang_id", "Bahasa Indonesia", "captive/language_code", "ID"));
  language->translate(Language_DE, "Sprache")->translate(Language_ID, "Bahasa");

  Menu* top = new Menu("Scale");
  Menu* settings = new Menu("Settings");
  settings->translate(Language_DE, "Einstellungen")->translate(Language_ID, "Pengaturan");
  settings->add(calibrate)->add(language);

  top->add(weight)->add(tare)->add(settings);
  top->translate(Language_DE, "Waage")->translate(Language_ID, "Timbangan");

  // ==== HOW it is driven, and WHERE it is shown ====
  Control_Menu* menu = (new Control_Menu_Simple("Menu", top))->addDisplay(new Menu_Display_Serial());
  #if defined(ACTUATOR_LCD_WANT)
    // While the menu is not in use the LCD lists the top menu - so it shows the weight
    frugal_iot.actuators->add(new Actuator_LCD());
    menu->addDisplay(new Menu_Display_LCD());
  #elif defined(CONTROL_MENU_LCD_NODE)
    // The LCD is on another node - wired to its Actuator_LCD's input, so sent over MQTT
    Menu_Display_LCD* lcd = new Menu_Display_LCD(ACTUATOR_LCD_COLS, ACTUATOR_LCD_ROWS, nullptr);
    lcd->out->wireTo(frugal_iot.org + "/" + frugal_iot.project + "/" + CONTROL_MENU_LCD_NODE + "/set/lcd/message");
    menu->addDisplay(lcd);
  #endif
  #ifdef ACTUATOR_OLED_WANT
    #ifdef SENSOR_LOADCELL_DOUTPIN
      // While the menu is not in use the OLED shows the weight large; the menu takes the screen when a key is pressed
      Control* weightDisplay = (new Control_Oled_Single("Control OLED", "g", "Weight"))->wireFrom(loadcell);
      frugal_iot.controls->add(weightDisplay);
      menu->addDisplay(new Menu_Display_OLED(weightDisplay));
    #else
      menu->addDisplay(new Menu_Display_OLED()); // Nothing to weigh, so home lists the top menu
    #endif
  #endif
  #ifdef CONTROL_MENU_CAPTIVE_WANT
    menu->addDisplay(new Menu_Display_Captive()); // On the portal page, with four buttons
  #endif
  frugal_iot.controls->add(menu);

  Sensor_Button* b;
  #ifdef SENSOR_BUTTON_ONE_PIN
    // One button: click for the next row (editing a number, the next one down), double click to
    // accept, hold to step - faster the longer it is held. Back is the "Back" row at the end of each
    // menu, there because back is not wired - a long press is not used, as releasing a hold is one.
    frugal_iot.buttons->add(b = new Sensor_Button("button", "Button", SENSOR_BUTTON_ONE_PIN, "black"));
    menu->down->wireFrom(b->singleClick);
    b->held->wireTo(menu->down);
    menu->accept->wireFrom(b->doubleClick);
  #else
    // Each button's click is a key press, wired from the menu's side so it knows which keys it has.
    // Holding up or down repeats, faster the longer it is held - pushed, as an input takes one source.
    frugal_iot.buttons->add(b = new Sensor_Button("button_up", "Up", SENSOR_BUTTON_UP_PIN, "black"));
    menu->up->wireFrom(b->singleClick);
    b->held->wireTo(menu->up);
    frugal_iot.buttons->add(b = new Sensor_Button("button_down", "Down", SENSOR_BUTTON_DOWN_PIN, "black"));
    menu->down->wireFrom(b->singleClick);
    b->held->wireTo(menu->down);
    frugal_iot.buttons->add(b = new Sensor_Button("button_accept", "Accept", SENSOR_BUTTON_ACCEPT_PIN, "black"));
    menu->accept->wireFrom(b->singleClick);
    #ifdef SENSOR_BUTTON_BACK_PIN
      frugal_iot.buttons->add(b = new Sensor_Button("button_back", "Back", SENSOR_BUTTON_BACK_PIN, "black"));
      menu->back->wireFrom(b->singleClick);
    #endif
  #endif

  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and system
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop();
}
