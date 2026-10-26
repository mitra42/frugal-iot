/* Frugal IoT - Control OLED Single - see control/oled_single.h */
#include "control/oled_single.h"
#include "Frugal-IoT.h"

#ifdef ACTUATOR_OLED_WANT // Only compile if have an OLED

Control_Oled_Single::Control_Oled_Single(const char* name, const char* unit, const char* label, uint8_t decimals)
  : Control_Oled("control_oled_single", name, std::vector<IN*> { }),
  value(new INfloat("control_oled_single", "value", "Value", 0, decimals, IO_DEFAULTS(control_oled_single_value), true)),
  unit(unit),
  label(label)
  {
    inputs.push_back(value);
  }

// Called when the input changes
void Control_Oled_Single::act() {
  if (enabled) {
    auto* display = &frugal_iot.oled->display; // auto: the concrete driver is chosen at compile time, see actuator/oled.h
    // 128x64 (and 128x128): value at size 3 (24px high), unit and label at 2.
    // 72x40 HW675: value at size 2 (16px high), unit and label at 1.
    const uint8_t big = (display->visibleHeight() >= 64) ? 3 : 2;
    const uint8_t small = big - 1;
    #ifdef ACTUATOR_OLED_DEBUG
      Serial.println(F("Writing fresh to oLED"));
    #endif
    display->clearDisplay();
    display->setCursor(0, 0);
    display->setTextSize(big);
    if (value->isValid()) {
      display->print(value->floatValue(), value->width);
    } else {
      display->print(F("--")); // Sensor published "nan" - rather than printing "nan"
    }
    if (unit) {
      // Smaller, and bottom-aligned with the value: a size n character is 8n pixels high
      display->setCursor(display->getCursorX(), 8 * (big - small));
      display->setTextSize(small);
      display->print(unit);
    }
    if (label) {
      display->setCursor(0, 8 * big + 4 * small); // Half a line of the smaller size below the value
      display->setTextSize(small);
      display->print(label);
    }
    display->display();
  }
}
#endif // ACTUATOR_OLED_WANT
