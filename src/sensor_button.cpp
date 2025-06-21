/* 
 * Frugal IoT button handler 
 * 
 * Detect a button click, and send a message depending on SINGLE, LONG, DOUBLE, or TRIPLE
 */

#include "_settings.h"

#ifdef SENSOR_BUTTON_WANT
#include <Arduino.h>
#include <vector>
#include "sensor.h"
#include "sensor_button.h"
#include "system_frugal.h"
#include "Button2.h" // https://github.com/LennartHennigs/Button2

Frugal_Group buttons("buttons", "Buttons");

Sensor_Button::Sensor_Button(const char * const id, const char * const name, uint8_t pin, const char* const color) :
  Sensor(id, name, false), pin(pin) {
  output = new OUTuint16(id, "out", name, empty, single_click, empty, color, false); // TODO convert this into a OUTenum - hard part is UX
  button = new Button2(pin);
  button->setClickHandler(Sensor_Button::clickHandler);
  button->setLongClickHandler(Sensor_Button::longClickHandler);
  button->setDoubleClickHandler(Sensor_Button::doubleClickHandler);
  button->setTripleClickHandler(Sensor_Button::tripleClickHandler);
  button->setID(buttons.group.size());
  buttons.group.push_back(this);
}

// Unclear how would use an "OUT" as its not dependent on a change
void Sensor_Button::clickHandlerInner(clickType type) {
  output->set(type);
}
void Sensor_Button::clickHandler(Button2& btn) {
  handler(btn)->clickHandlerInner(single_click);
}

void Sensor_Button::longClickHandler(Button2& btn) {
  handler(btn)->clickHandlerInner(long_click);
}

void Sensor_Button::doubleClickHandler(Button2& btn) {
  handler(btn)->clickHandlerInner(double_click);
}

void Sensor_Button::tripleClickHandler(Button2& btn) {
  handler(btn)->clickHandlerInner(triple_click);
}
Sensor_Button* Sensor_Button::handler(Button2& button) {
  return (Sensor_Button*)buttons.group[button.getID()];
}
void Sensor_Button::setup() {
  button->begin(pin);
}
void Sensor_Button::frequently() {
  button->loop(); // TODO-141 should probably only do every 10 MS 
}


#endif // SYSTEM_BUTTON_WANT
