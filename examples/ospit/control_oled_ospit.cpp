/* See control_oled_ospit.h - three pages in a carousel, ported from OSPIT's display.lua */

#include "control_oled_ospit.h"

#ifdef ACTUATOR_OLED_WANT

#include "Frugal-IoT.h"
#include <WiFi.h>
#include <cmath>

// A reading that is not there prints as "--", not as a number that looks real. OSPIT prints its
// -127 sentinel here, which is honest but asks the reader to know what -127 means.
static void printValue(Print* out, INfloat* in, uint8_t width) {
  if (in->isValid()) {
    out->print(in->floatValue(), width);
  } else {
    out->print(F("--"));
  }
}

// ---- Page 1: battery ------------------------------------------------------------------

Control_Oled_OspitPower::Control_Oled_OspitPower()
  : Control_Oled("oledpower", "Display power", std::vector<IN*>{}),
    battery(new INfloat("oledpower", "battery", "Battery", NAN, 0, 0, 15000, "#008000", true))
{
  inputs.push_back(battery);
}

void Control_Oled_OspitPower::act() {
  if (enabled) {
    auto* display = &frugal_iot.oled->display; // auto: the driver is chosen at compile time
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(OLED_FG);
    display->setCursor(0, 0);
    display->print(F("Battery"));
    display->setTextSize(2);
    display->setCursor(0, 16);
    if (battery->isValid()) {
      display->print(battery->floatValue() / 1000.0f, 2); // millivolts in, volts on screen
      display->print(F("V"));
    } else {
      display->print(F("--"));
    }
    display->setTextSize(1);
    display->setCursor(0, 48);
    // TODO P4/P5 this is where OSPIT's solar open-circuit voltage, MPP tracking voltage, charge
    // state and battery temperature go - display.lua page 1. None of it exists yet.
    display->print(F("MPPT: not fitted"));
    display->display();
  }
}

// ---- Page 2: sectors and tank ---------------------------------------------------------

Control_Oled_OspitSoil::Control_Oled_OspitSoil()
  : Control_Oled("oledsoil", "Display soil", std::vector<IN*>{}),
    moisture1(new INfloat("oledsoil", "moisture1", "Moisture 1", NAN, 0, 0, 100, "#a52a2a", true)),
    moisture2(new INfloat("oledsoil", "moisture2", "Moisture 2", NAN, 0, 0, 100, "#a52a2a", true)),
    moisture3(new INfloat("oledsoil", "moisture3", "Moisture 3", NAN, 0, 0, 100, "#a52a2a", true)),
    tank(new INfloat("oledsoil", "tank", "Tank", NAN, 0, 0, 100, "#0000ff", true)),
    active(new INuint16("oledsoil", "active", "Active", 0, 0, 16, 0, 16, "#000000", true))
{
  inputs.push_back(moisture1);
  inputs.push_back(moisture2);
  inputs.push_back(moisture3);
  inputs.push_back(tank);
  inputs.push_back(active);
}

void Control_Oled_OspitSoil::act() {
  if (enabled) {
    auto* display = &frugal_iot.oled->display;
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(OLED_FG);
    INfloat* m[3] = { moisture1, moisture2, moisture3 };
    for (uint8_t i = 0; i < 3; i++) {
      display->setCursor(0, i * 10);
      display->print(F("Sector "));
      display->print(i + 1);
      // A marker beside the sector currently watering - nothing OSPIT shows, but it is the
      // question anyone standing at the box is actually asking
      display->print((active->value == (i + 1)) ? F(" *") : F("  "));
      display->setCursor(72, i * 10);
      printValue(display, m[i], 0);
      display->print(F("%"));
    }
    display->setCursor(0, 40);
    display->print(F("Tank"));
    display->setCursor(72, 40);
    printValue(display, tank, 0);   // "--" when no sender is fitted, which OSPIT reads as empty
    display->print(F("%"));
    display->display();
  }
}

// ---- Page 3: connectivity -------------------------------------------------------------

Control_Oled_OspitNet::Control_Oled_OspitNet()
  : Control_Oled("olednet", "Display network", std::vector<IN*>{})
{ }

void Control_Oled_OspitNet::act() {
  if (enabled) {
    auto* display = &frugal_iot.oled->display;
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(OLED_FG);
    const bool up = (WiFi.status() == WL_CONNECTED);
    display->setCursor(0, 0);
    display->print(F("IP   "));
    display->print(up ? WiFi.localIP().toString() : String(F("no-ip")));
    display->setCursor(0, 10);
    display->print(F("WiFi "));
    display->print(up ? WiFi.SSID() : String(F("disconnected")));
    display->setCursor(0, 20);
    display->print(F("RSSI "));
    display->print(up ? String(WiFi.RSSI()) : String(F("-")));
    display->setCursor(0, 30);
    display->print(F("MQTT "));
    display->print(frugal_iot.mqtt->connected() ? F("on") : F("off"));
    display->setCursor(0, 40);
    display->print(F("Heap "));
    display->print(ESP.getFreeHeap());
    display->display();
  }
}

// ---- Assembly -------------------------------------------------------------------------

Control_Carousel* ospitDisplay() {
  Control_Oled_OspitPower* page1 = new Control_Oled_OspitPower();
  Control_Oled_OspitSoil*  page2 = new Control_Oled_OspitSoil();
  Control_Oled_OspitNet*   page3 = new Control_Oled_OspitNet();
  frugal_iot.controls->add(page1);
  frugal_iot.controls->add(page2);
  frugal_iot.controls->add(page3);
  // Only one page draws at a time; the carousel flips `enabled` as the selection moves
  page2->enabled = false;
  page3->enabled = false;

  Control_Carousel* carousel = new Control_Carousel("Display");
  frugal_iot.controls->add(carousel);
  carousel->controls.push_back(page1);
  carousel->controls.push_back(page2);
  carousel->controls.push_back(page3);
  return carousel;
}

#endif // ACTUATOR_OLED_WANT
