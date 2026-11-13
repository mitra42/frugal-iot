/* 
 * Frugal IoT button handler 
 * 
 * Detect a button click, and send a message depending on SINGLE, LONG, DOUBLE, or TRIPLE
 */

#include "_settings.h"
#include <Arduino.h>
#include <vector>
#include "sensor/sensor.h"
#include "sensor/button.h"
#include "system/frugal.h"
#include "Button2.h" // https://github.com/LennartHennigs/Button2

// Map a button id (inside a Button2) to a Sensor_Button (from the frugal_iot.buttons group).
Sensor_Button* button(Button2& button) {
  return (Sensor_Button*)frugal_iot.buttons->group[button.getID()];
}

/* A press is an event, so every one is sent - not through OUT::set(), which only sends when the
 * value changes, and a click is always "1". Sent both ways, as set() would:
 *   on its own topic, e.g. button/click - so an IN can wireFrom() it, on this node or another.
 *     Not retained: a retained click would be replayed as a fresh press to whatever subscribes
 *     later, e.g. at its next boot.
 *   to its wiredPath, if it has been wireTo()-ed somewhere - the older way of wiring a button.
 * Wire a button one way or the other, not both, or each press arrives twice.
 */
static void press(OUTtext* out) {
  frugal_iot.messages->send(out->path(), out->StringValue(), MQTT_DONT_RETAIN, MQTT_QOS_ATLEAST1); // published, and looped back
  out->sendWired(MQTT_DONT_RETAIN, MQTT_QOS_EXACTLY1);
}

void clickHandler(Button2& btn) {
  press(button(btn)->singleClick);
}

void longClickHandler(Button2& btn) {
  press(button(btn)->longClick);
}

void doubleClickHandler(Button2& btn) {
  press(button(btn)->doubleClick);
}

void tripleClickHandler(Button2& btn) {
  press(button(btn)->tripleClick);
}

// Called once when a long click is first detected, then every SENSOR_BUTTON_REPEAT_MS while held
void heldHandler(Button2& btn) {
  OUTtext* held = button(btn)->held;
  held->value = String(btn.getLongClickCount()); // 1, 2, 3 ... so the receiver can accelerate
  press(held);
}


Sensor_Button::Sensor_Button(const char * const id, const char * const name, uint8_t pin, const char* const color) :
  System_Base(id, name), pin(pin) {
  //output = new OUTuint16(id, "out", name, empty, single_click, empty, color, false); // TODO convert this into a OUTenum - hard part is UX
  singleClick = new OUTtext(id, "click", "Single Click", "1", "black", true);
  longClick = new OUTtext(id, "long", "Long Click", "1", "black", true);
  doubleClick = new OUTtext(id, "double", "Double CLick", "1", "black", true);
  tripleClick = new OUTtext(id, "triple", "Triple Click", "1", "black", true);
  held = new OUTtext(id, "held", "Held", "1", "black", true);
  button = new Button2(pin);
  button->setClickHandler(clickHandler);
  button->setLongClickHandler(longClickHandler);
  button->setDoubleClickHandler(doubleClickHandler);
  button->setTripleClickHandler(tripleClickHandler);
  button->setLongClickDetectedHandler(heldHandler);
  button->setLongClickDetectedRetriggerable(true, SENSOR_BUTTON_REPEAT_MS);
  button->setID(frugal_iot.buttons->group.size());
}

void Sensor_Button::setup() {
  System_Base::setup(); // Will readConfigFromFS - do before setting up pins
  button->begin(pin);
}
void Sensor_Button::loop() {
  button->loop(); // loop runs with a delay of 10ms so this is about right
}
