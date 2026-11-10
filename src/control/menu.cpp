/* Frugal IoT - local menu UX - see control/menu.h */
#include "control/menu.h"
#include <algorithm>
#include <string.h>
#include "Frugal-IoT.h"

// ==== MenuText ======================================================

MenuText::MenuText(const char* en) : en(en) {}

void MenuText::add(Language_t lang, const char* text) {
  others.push_back(std::make_pair(lang, text));
}

const char* MenuText::get() const {
  const char* found = en;
  for (auto &other : others) {
    if (TT[other.first] == T) { // T is the current language's table
      found = other.second;
    }
  }
  return found;
}

// ==== MenuItem and subclasses =======================================

MenuItem::MenuItem(MenuKind kind, const char* text) : kind(kind), text(text) {}

MenuItem* MenuItem::translate(Language_t lang, const char* t) {
  text.add(lang, t);
  return this;
}

String MenuItem::valueText() {
  return String();
}

bool MenuItem::visit(std::vector<MenuItem*>& visited) {
  const bool fresh = (std::find(visited.begin(), visited.end(), this) == visited.end());
  if (fresh) {
    visited.push_back(this);
  }
  return fresh;
}

void MenuItem::collect(std::vector<IN*>& ins, std::vector<OUT*>& outs, std::vector<MenuItem*>& visited) {
  visit(visited); // Nothing of its own to collect
}

Menu::Menu(const char* text) : MenuItem(MENU_MENU, text) {}

Menu* Menu::add(MenuItem* item) {
  items.push_back(item);
  return this;
}

void Menu::collect(std::vector<IN*>& ins, std::vector<OUT*>& outs, std::vector<MenuItem*>& visited) {
  if (visit(visited)) { // A menu can link back up the tree, so only walk each one once
    for (auto &i : items) {
      i->collect(ins, outs, visited);
    }
  }
}

// The live input's id is "<id>_live". IO keeps the pointer, and items last for ever, so this is never freed.
static const char* liveId(const char* id) {
  return strdup((String(id) + "_live").c_str());
}

MenuLine::MenuLine(const char* id, const char* text, const char* topicTwig, const char* payload)
  : MenuItem(MENU_LINE, text),
  out(new OUTtext("menu", id, text, "", "#000000", true)),
  live(new INtext("menu", liveId(id), text, "", "#000000", true)),
  payload(payload)
{
  if (topicTwig) {
    out->wireTo(frugal_iot.messages->setPath(topicTwig));
  }
}

String MenuLine::valueText() {
  String s;
  if (live->wiredPath.length()) {
    s = live->value;
    if (live->unit) {
      s += " ";
      s += live->unit;
    }
  }
  return s;
}

String MenuLine::acceptPayload() {
  String p;
  if (payload) {
    p = payload;
  } else if (live->wiredPath.length()) {
    p = live->value; // Take the reading and send it
  } else {
    p = "1";
  }
  return p;
}

void MenuLine::collect(std::vector<IN*>& ins, std::vector<OUT*>& outs, std::vector<MenuItem*>& visited) {
  if (visit(visited)) {
    ins.push_back(live);
    outs.push_back(out);
  }
}

MenuNumber::MenuNumber(const char* id, const char* text, const char* topicTwig,
    float value, float min, float max, float step, uint8_t decimals)
  : MenuItem(MENU_NUMBER, text),
  out(new OUTtext("menu", id, text, "", "#000000", true)),
  live(new INfloat("menu", liveId(id), text, value, decimals, min, max, "#000000", true)),
  min(min), max(max), step(step), decimals(decimals)
{
  if (topicTwig) {
    out->wireTo(frugal_iot.messages->setPath(topicTwig));
  }
}

String MenuNumber::format(float v) {
  return String(v, (unsigned int)decimals);
}

String MenuNumber::valueText() {
  String s = live->isValid() ? format(live->floatValue()) : String(F("--"));
  if (live->unit) {
    s += " ";
    s += live->unit;
  }
  return s;
}

void MenuNumber::collect(std::vector<IN*>& ins, std::vector<OUT*>& outs, std::vector<MenuItem*>& visited) {
  if (visit(visited)) {
    ins.push_back(live);
    outs.push_back(out);
  }
}

// ==== Menu_Display ==================================================

String Menu_Display::fit(const String& left, const String& right, uint8_t width) {
  const unsigned int gap = right.length() ? 1 : 0;
  const unsigned int room = (right.length() + gap < width) ? (width - right.length() - gap) : 0;
  String s = (left.length() > room) ? left.substring(0, room) : left;
  if (right.length()) {
    while (s.length() + right.length() < width) {
      s += ' ';
    }
    s += right;
  }
  return s.substring(0, width);
}

uint8_t Menu_Display::firstRow(uint8_t row, uint8_t visible) {
  return (visible == 0 || row < visible) ? 0 : (uint8_t)(row - visible + 1);
}

String Menu_Display::rowValue(Control_Menu* m, uint8_t r) {
  MenuItem* i = m->item(r);
  String v;
  if (i) {
    v = (i->kind == MENU_MENU) ? String(">") : i->valueText();
  }
  return v;
}

// ==== Menu_Display_Serial ===========================================

void Menu_Display_Serial::render(Control_Menu* m) {
  if (!m->active) {
    if (!homeShown) {
      Serial.println(F("[menu] home"));
      homeShown = true;
    }
  } else if (m->editing) {
    Serial.print(F("[menu] ")); Serial.print(m->title()); Serial.print(F(": "));
    Serial.print(m->editing->format(m->editValue));
    Serial.print(F("  (")); Serial.print(m->editing->format(m->editing->min));
    Serial.print(F("..")); Serial.print(m->editing->format(m->editing->max)); Serial.println(F(")"));
  } else {
    homeShown = false;
    Serial.print(F("[menu] ")); Serial.println(m->title());
    for (uint8_t r = 0; r < m->rows(); r++) {
      Serial.print(r == m->row ? F("  > ") : F("    "));
      Serial.print(m->rowText(r));
      MenuItem* i = m->item(r);
      if (i) {
        String v = i->valueText();
        if (v.length()) {
          Serial.print(F("  ")); Serial.print(v);
        }
        if (i->kind == MENU_MENU) {
          Serial.print(F(" >"));
        }
      }
      Serial.println();
    }
  }
}

// ==== Control_Menu ==================================================

Control_Menu::Control_Menu(const char* name, Menu* top)
  : Control("menu", name, std::vector<IN*>{}, std::vector<OUT*>{}),
  up(new INtext("menu", "up", "Up", "", "#000000", true)),
  down(new INtext("menu", "down", "Down", "", "#000000", true)),
  accept(new INtext("menu", "accept", "Accept", "", "#000000", true)),
  back(new INtext("menu", "back", "Back", "", "#000000", true)),
  top(top),
  menu(top)
{
  inputs.push_back(up);
  inputs.push_back(down);
  inputs.push_back(accept);
  inputs.push_back(back);
  // The items' IOs become this control's, so they are set up, dispatched, discovered and listed on /status
  std::vector<MenuItem*> visited;
  top->collect(inputs, outputs, visited);
}

Control_Menu* Control_Menu::addDisplay(Menu_Display* display) {
  displays.push_back(display);
  OUT* o = display->output();
  if (o) {
    outputs.push_back(o);
  }
  return this;
}

void Control_Menu::setup() {
  Control::setup(); // Reads wiring from the filesystem - so only after it is back known to be wired or not
  backrow = (back->wiredPath.length() == 0);
  render(); // Home
}

void Control_Menu::loop() {
  #if CONTROL_MENU_TIMEOUT_MS > 0
    if (active && (millis() - lastKey > CONTROL_MENU_TIMEOUT_MS)) {
      if (editing) {
        endEdit(false);
      }
      goHome();
      render();
    }
  #endif
}

/* The keys are events: a button sends "1" on every press, and IN only reports a change when
 * the value changes, so a second press of the same key would never reach act(). So catch them
 * here, either sent to menu/<key> (a button wired to the input) or arriving on the input's own
 * wiredPath (the input wired from a button). Everything else - live values, wiring - goes on to
 * Control::dispatch.
 */
void Control_Menu::dispatch(System_Message &msg) {
  INtext* key = nullptr;
  for (INtext* k : { up, down, accept, back }) {
    if ((msg.module() == id && msg.leaf() == k->id)
        || (k->wiredPath.length() && msg.topicPath == k->wiredPath)) {
      key = k;
    }
  }
  if (key && !msg.isFromFS()) {
    long count = msg.payload.toInt(); // Sensor_Button's "held" counts up; a click is "1"
    if (count < 1) {
      count = 1;
    }
    lastKey = millis();
    #ifdef CONTROL_MENU_DEBUG
      Serial.print(F("menu key ")); Serial.print(key->id); Serial.print(F(" x")); Serial.println(count);
    #endif
    if (!active) {
      active = true; // The first press only wakes the menu, at the top
      menu = top;
      row = 0;
      depth = 0;
    } else if (key == up) {
      keyUp((uint16_t)count);
    } else if (key == down) {
      keyDown((uint16_t)count);
    } else if (key == accept) {
      keyAccept();
    } else {
      keyBack();
    }
    render();
  } else if (!key) {
    Control::dispatch(msg);
  }
}

void Control_Menu::act() {
  render(); // A live value changed - even at home, where a display may be showing it
}

uint8_t Control_Menu::rows() {
  return (uint8_t)(menu->items.size() + (backrow ? 1 : 0));
}

bool Control_Menu::isBackRow(uint8_t r) {
  return backrow && (r == menu->items.size());
}

MenuItem* Control_Menu::item(uint8_t r) {
  return (r < menu->items.size()) ? menu->items[r] : nullptr;
}

const char* Control_Menu::title() {
  return editing ? editing->text.get() : menu->text.get();
}

String Control_Menu::rowText(uint8_t r) {
  MenuItem* i = item(r);
  return i ? String(i->text.get()) : String(T->Back); // T->Back is a flash string on ESP8266
}

void Control_Menu::open(Menu* m) {
  int8_t found = -1;
  for (uint8_t i = 0; i < depth; i++) {
    if (history[i].menu == m) {
      found = i;
    }
  }
  if (found >= 0) {
    depth = (uint8_t)found; // Already on the way here - go back to it rather than loop round
  } else if (m != menu) {
    if (depth == CONTROL_MENU_DEPTH) { // Full - forget the oldest
      for (uint8_t i = 1; i < CONTROL_MENU_DEPTH; i++) {
        history[i - 1] = history[i];
      }
      depth--;
    }
    history[depth++] = HistoryEntry{ menu, row };
  }
  menu = m;
  row = 0;
}

void Control_Menu::close() {
  if (depth > 0) {
    depth--;
    menu = history[depth].menu;
    row = history[depth].row;
  } else {
    goHome();
  }
}

void Control_Menu::goHome() {
  active = false;
  editing = nullptr;
  menu = top;
  row = 0;
  depth = 0;
}

static void sendOut(OUTtext* out, const String& payload) {
  out->value = payload;
  out->send(); // Published on menu/<id>, so what was chosen is recorded
  out->sendWired(MQTT_DONT_RETAIN, MQTT_QOS_EXACTLY1); // and to its topic, as a button does
}

void Control_Menu::sendLine(MenuLine* line) {
  sendOut(line->out, line->acceptPayload());
}

void Control_Menu::startEdit(MenuNumber* n) {
  float v = n->live->isValid() ? n->live->floatValue() : n->min;
  if (v < n->min) { v = n->min; }
  if (v > n->max) { v = n->max; }
  editing = n;
  editStart = v;
  editValue = v;
}

void Control_Menu::sendNumber() {
  sendOut(editing->out, editing->format(editValue));
}

void Control_Menu::endEdit(bool keep) {
  if (keep) {
    if (changed(editValue, editStart)) {
      if (!editing->sendEachChange) { // Otherwise already sent
        sendNumber();
      }
      editing->live->set(editValue); // So, unwired, the next edit starts here; wired, the source will update it anyway
    }
  } else if (editing->sendEachChange && changed(editValue, editStart)) {
    editValue = editStart; // Put back what was there
    sendNumber();
  }
  editing = nullptr;
}

void Control_Menu::captiveLines(AsyncResponseStream* response) {
  for (auto &d : displays) {
    d->captiveLines(this, response);
  }
}

// The fastest any display asks for, 0 if none wants refreshing
unsigned long Control_Menu::captiveRefreshMs() {
  unsigned long ms = 0;
  for (auto &d : displays) {
    const unsigned long m = d->captiveRefreshMs();
    if (m && (!ms || m < ms)) {
      ms = m;
    }
  }
  return ms;
}

void Control_Menu::captiveValues(Captive_Values* values, const String& moduleId) {
  if (moduleId == id) {
    for (auto &d : displays) {
      d->captiveValues(this, values);
    }
  }
}

void Control_Menu::render() {
  for (auto &d : displays) {
    d->render(this);
  }
}

// ==== Control_Menu_Simple ===========================================

Control_Menu_Simple::Control_Menu_Simple(const char* name, Menu* top)
  : Control_Menu(name, top) {}

void Control_Menu_Simple::step(int8_t direction, uint16_t count) {
  if (editing) {
    // Held down, go faster: x10 after 10 repeats, x100 after 20
    const float mult = (count >= 20) ? 100 : ((count >= 10) ? 10 : 1);
    float v = editValue + direction * editing->step * mult;
    const float p = powf(10, editing->decimals);
    v = roundf(v * p) / p; // So repeated steps of 0.1 do not drift
    if (v > editing->max) { v = editing->min; } // Wrap round at either end
    if (v < editing->min) { v = editing->max; }
    editValue = v;
    if (editing->sendEachChange) {
      sendNumber();
    }
  } else {
    const uint8_t n = rows();
    if (n) {
      if (direction > 0) {
        row = (row + 1) % n;
      } else {
        row = (row == 0) ? (n - 1) : (row - 1);
      }
    }
  }
}

// Up: previous row, or increase the number being edited
void Control_Menu_Simple::keyUp(uint16_t count) {
  step(editing ? 1 : -1, count);
}

// Down: next row, or decrease the number being edited
void Control_Menu_Simple::keyDown(uint16_t count) {
  step(editing ? -1 : 1, count);
}

// Accept: open a menu, send a line, start editing a number - or, editing, keep the number
void Control_Menu_Simple::keyAccept() {
  if (editing) {
    endEdit(true);
  } else if (isBackRow(row)) {
    close();
  } else {
    MenuItem* i = item(row);
    if (i) {
      switch (i->kind) {
        case MENU_MENU:   open((Menu*)i); break;
        case MENU_LINE:   sendLine((MenuLine*)i); break;
        case MENU_NUMBER: startEdit((MenuNumber*)i); break;
      }
    }
  }
}

// Back: leave the number unchanged, or go back a menu - from the top, go home
void Control_Menu_Simple::keyBack() {
  if (editing) {
    endEdit(false);
  } else {
    close();
  }
}
