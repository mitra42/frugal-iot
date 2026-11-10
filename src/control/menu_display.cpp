/* Frugal IoT - displays for Control_Menu - see control/menu_display.h */
#include "control/menu_display.h"
#include "Frugal-IoT.h"
#include "system/captive.h" // html_entities, Captive_Values

// ==== Menu_Display_LCD ==============================================

Menu_Display_LCD::Menu_Display_LCD(uint8_t cols, uint8_t rows, const char* topicTwig)
  : out(new OUTtext("menu", "lcd", "LCD", "", "#000000", true)), cols(cols), rows(rows)
{
  if (topicTwig) {
    out->wireTo(frugal_iot.messages->setPath(topicTwig));
  }
}

void Menu_Display_LCD::render(Control_Menu* m) {
  String text;
  if (m->editing) {
    text = Menu_Display::fit(m->title(), "", cols) + "\n"
      + Menu_Display::fit(m->editing->format(m->editValue), m->editing->live->unit ? m->editing->live->unit : "", cols);
    if (rows > 2) {
      text += "\n" + Menu_Display::fit(m->editing->format(m->editing->min) + ".." + m->editing->format(m->editing->max), "", cols);
    }
  } else {
    text = Menu_Display::fit(m->title(), "", cols);
    const uint8_t visible = rows - 1; // Under the title
    const uint8_t first = Menu_Display::firstRow(m->row, visible);
    for (uint8_t r = first; (r < m->rows()) && (r < first + visible); r++) {
      // At home there is no cursor - it is just showing the top menu and its values
      const char* marker = (m->active && (r == m->row)) ? ">" : " ";
      text += "\n" + Menu_Display::fit(marker + m->rowText(r), Menu_Display::rowValue(m, r), cols);
    }
  }
  if (text != last) {
    last = text;
    out->value = text;
    out->sendWired(MQTT_DONT_RETAIN, MQTT_QOS_EXACTLY1); // Locally if it is this node's LCD, otherwise over MQTT
  }
}

// ==== Menu_Display_OLED =============================================

#ifdef ACTUATOR_OLED_WANT

Menu_Display_OLED::Menu_Display_OLED(Control* home) : home(home) {}

void Menu_Display_OLED::render(Control_Menu* m) {
  if (m->active) {
    if (home) {
      home->enabled = false; // So it does not draw over the menu when its value changes
    }
    if (m->editing) {
      drawEdit(m);
    } else {
      drawMenu(m, true);
    }
  } else if (home) {
    if (!home->enabled) { // Just gone home - give the screen back
      home->enabled = true;
      home->act();
    }
  } else {
    drawMenu(m, false);
  }
}

// Text at size 1 is 6x8 pixels a character
static const uint8_t charW = 6;

// Title in an inverted bar across the top - returns the height it took
static int16_t drawTitle(const char* title) {
  auto* display = &frugal_iot.oled->display; // auto: the concrete driver is chosen at compile time, see actuator/oled.h
  const int16_t h = 9;
  display->fillRect(0, 0, display->visibleWidth(), h, OLED_FG);
  display->setTextColor(OLED_BG);
  display->setCursor(1, 1);
  frugal_iot.oled->printUTF8(Menu_Display::fit(title, "", display->visibleWidth() / charW).c_str());
  display->setTextColor(OLED_FG);
  return h + 1;
}

void Menu_Display_OLED::drawMenu(Control_Menu* m, bool cursor) {
  auto* display = &frugal_iot.oled->display;
  display->clearDisplay();
  display->setTextSize(1);
  const int16_t top = drawTitle(m->title());
  const int16_t lineH = (display->visibleHeight() >= 64) ? 9 : 8; // A little air where there is room
  const uint8_t visible = (display->visibleHeight() - top) / lineH;
  const uint8_t indent = 8; // Room for the triangle
  const uint8_t width = (display->visibleWidth() - indent) / charW;
  const uint8_t first = Menu_Display::firstRow(m->row, visible);
  for (uint8_t r = first; (r < m->rows()) && (r < first + visible); r++) {
    const int16_t y = top + (r - first) * lineH;
    if (cursor && (r == m->row)) {
      display->fillTriangle(0, y, 0, y + 6, 4, y + 3, OLED_FG);
    }
    display->setCursor(indent, y);
    frugal_iot.oled->printUTF8(Menu_Display::fit(m->rowText(r), Menu_Display::rowValue(m, r), width).c_str());
  }
  display->display();
}

void Menu_Display_OLED::drawEdit(Control_Menu* m) {
  auto* display = &frugal_iot.oled->display;
  MenuNumber* n = m->editing;
  display->clearDisplay();
  display->setTextSize(1);
  const int16_t top = drawTitle(m->title());
  // As big as fits: size 3 is 18 pixels a character, size 2 is 12
  const String v = n->format(m->editValue);
  const uint8_t big = ((v.length() * 3 * charW) <= (unsigned)display->visibleWidth() && display->visibleHeight() >= 64) ? 3 : 2;
  display->setTextSize(big);
  display->setCursor(0, top + 2);
  display->print(v);
  if (n->live->unit) {
    display->setTextSize(1);
    display->setCursor(display->getCursorX() + 2, top + 2 + 8 * (big - 1)); // Bottom-aligned with the value
    frugal_iot.oled->printUTF8(n->live->unit);
  }
  display->setTextSize(1);
  display->setCursor(0, display->visibleHeight() - 8);
  display->print(n->format(n->min) + ".." + n->format(n->max));
  display->display();
}

#endif // ACTUATOR_OLED_WANT

// ==== Menu_Display_Captive ==========================================

String Menu_Display_Captive::screen(Control_Menu* m) {
  String h = String(F("<b>")) + html_entities(m->title()) + F("</b>");
  if (m->editing) {
    const char* unit = m->editing->live->unit;
    h += String(F("<div style='font-size:2em'>")) + m->editing->format(m->editValue)
      + (unit ? String(" ") + html_entities(unit) : String()) + F("</div><small>")
      + m->editing->format(m->editing->min) + F(" .. ") + m->editing->format(m->editing->max) + F("</small>");
  } else {
    h += F("<table>");
    for (uint8_t r = 0; r < m->rows(); r++) {
      // At home there is no cursor, as on the other displays
      const bool here = m->active && (r == m->row);
      h += String(F("<tr><td>")) + (here ? F("&#9654;") : F("")) + F("</td><td>")
        + (here ? F("<b>") : F("")) + html_entities(m->rowText(r)) + (here ? F("</b>") : F(""))
        + F("</td><td align=right>") + html_entities(Menu_Display::rowValue(m, r)) + F("</td></tr>");
    }
    h += F("</table>");
  }
  return h;
}

// One of the four buttons. After sending, fetch the screen at once rather than at the next refresh -
// r() is the page's refresh function, see System_Captive
static String keyButton(const char* key, const String& label) {
  return String(F("<input type=button style='width:4em' value=\"")) + label + F("\" onclick=\"s('menu/") + key
    + F("','1');let d=this.closest('[data-m]');if(d&&typeof r=='function'){setTimeout(()=>r(d),300)}\">");
}

void Menu_Display_Captive::captiveLines(Control_Menu* m, AsyncResponseStream* response) {
  response->print(String(F("<p>")) + html_entities(m->name) + F("</p><div data-t='menu/screen' data-h>") + screen(m) + F("</div>"));
  // A diamond: up at the top, back and accept either side, down below
  response->print(String(F("<div style='display:grid;grid-template-columns:repeat(3,4.5em);gap:4px;margin:8px 0'><span></span>"))
    + keyButton("up", F("&#9650;")) + F("<span></span>")
    + keyButton("back", html_entities(String(T->Back))) + F("<span></span>") + keyButton("accept", F("OK"))
    + F("<span></span>") + keyButton("down", F("&#9660;")) + F("<span></span></div>"));
}

void Menu_Display_Captive::captiveValues(Control_Menu* m, Captive_Values* values) {
  values->add(String(F("menu/screen")), screen(m));
}
