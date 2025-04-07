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
#include "Button2.h" // https://github.com/LennartHennigs/Button2
#include "system_mqtt.h"



Sensor_Button::Sensor_Button(uint8_t pin, const char* topicLeaf) :
  Sensor(topicLeaf, 10, false), pin(pin) {
  button = new Button2(pin);
  button->setClickHandler(Sensor_Button::clickHandler);
  button->setLongClickHandler(Sensor_Button::longClickHandler);
  button->setDoubleClickHandler(Sensor_Button::doubleClickHandler);
  button->setTripleClickHandler(Sensor_Button::tripleClickHandler);
}

// Unclear how would use an "OUT" as its not dependent on a change
void Sensor_Button::clickHandlerInner(clickType type) {
  Mqtt->messageSend(topicLeaf, type, retain, qos);
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
void Sensor_Button::newSensor_Button(uint8_t pin, const char* topicLeaf) {
  Sensor_Button* sb = new Sensor_Button(pin, topicLeaf);
  sb->button->setID(buttons.size());
  buttons.push_back(sb);
  sensors.push_back(sb);   
}
Sensor_Button* Sensor_Button::handler(Button2& button) {
  return buttons[button.getID()];
}
void Sensor_Button::setup() {
  button->begin(pin);
}
void Sensor_Button::loop() {
  button->loop();
}

std::vector<Sensor_Button*> buttons;

#endif // SYSTEM_BUTTON_WANT
