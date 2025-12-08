/* 
 * Frugal IoT button handler 
 * 
 * Detect a button click, and send a message depending on SINGLE, LONG, DOUBLE, or TRIPLE
 */

#include "_settings.h"
#include <Arduino.h>
#include <vector>
#include "sensor.h"
#include "sensor_button.h"
#include "system_frugal.h"
#include "Button2.h" // https://github.com/LennartHennigs/Button2

// Map a button id (inside a Button2) to a Sensor_Button (from the frugal_iot.buttons group).
Sensor_Button* button(Button2& button) {
  return (Sensor_Button*)frugal_iot.buttons->group[button.getID()];
}

void clickHandler(Button2& btn) {
  button(btn)->singleClick->sendWired();
}

void longClickHandler(Button2& btn) {
  button(btn)->longClick->sendWired();
}

void doubleClickHandler(Button2& btn) {
  button(btn)->doubleClick->sendWired();
}

void tripleClickHandler(Button2& btn) {
  button(btn)->tripleClick->sendWired();
}


Sensor_Button::Sensor_Button(const char * const id, const char * const name, uint8_t pin, const char* const color) :
  System_Base(id, name), pin(pin) {
  //output = new OUTuint16(id, "out", name, empty, single_click, empty, color, false); // TODO convert this into a OUTenum - hard part is UX
  singleClick = new OUTuint16(id, "click", "Single Click", 0, 0, 0xFFFF, "black", true);
  longClick = new OUTuint16(id, "long", "Long Click", 0, 0, 0xFFFF, "black", true);
  doubleClick = new OUTuint16(id, "double", "Double CLick", 0, 0, 0xFFFF, "black", true);
  tripleClick = new OUTuint16(id, "triple", "Triple Click", 0, 0, 0xFFFF, "black", true);
  button = new Button2(pin);
  button->setClickHandler(clickHandler);
  button->setLongClickHandler(longClickHandler);
  button->setDoubleClickHandler(doubleClickHandler);
  button->setTripleClickHandler(tripleClickHandler);
  button->setID(frugal_iot.buttons->group.size());
}

// Unclear how would use an "OUT" as its not dependent on a change
void Sensor_Button::clickHandlerInner(const clickType type) {
  output->set(type);
}
void Sensor_Button::clickHandler(const Button2& btn) {
  handler(btn)->clickHandlerInner(single_click);
}

void Sensor_Button::longClickHandler(const Button2& btn) {
  handler(btn)->clickHandlerInner(long_click);
}

void Sensor_Button::doubleClickHandler(const Button2& btn) {
  handler(btn)->clickHandlerInner(double_click);
}

void Sensor_Button::tripleClickHandler(const Button2& btn) {
  handler(btn)->clickHandlerInner(triple_click);
}
// Map a button id (inside a Button2) to a Sensor_Button (from the frugal_iot.buttons group).
Sensor_Button* Sensor_Button::handler(const Button2& button) {
  return (Sensor_Button*)frugal_iot.buttons->group[button.getID()];
}
void Sensor_Button::setup() {
  System_Base::setup(); // Will readConfigFromFS - do before setting up pins
  button->begin(pin);
}
void Sensor_Button::loop() {
  button->loop(); // loop runs with a delay of 10ms so this is about right
}
