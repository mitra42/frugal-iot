/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  Based on the Blinken demo in the IDE

  Required CONTROL_BLINKEN_S // Initial time, note overridden by message

  TODO-43 - allow the destination to be set - e.g. some other digital pin
  TODO-43 - allow the input to be routed from somewhere (e.g. from temperature or a potentiometer )

 */

#include "_settings.h"  // Settings for what to include etc

#ifdef CONTROL_BLINKEN_WANT

#if (!defined(CONTROL_BLINKEN_S))
  // Setting default blink of 1 second, override in _local.h. 
  #define CONTROL_BLINKEN_S 1
#endif

#include <Arduino.h>
#include "control_blinken.h"
#include "system_discovery.h"
#include "system_mqtt.h"

namespace cBlinken {
const char* const inputTopic = "control_blinken_seconds";
const char* outputTopic = "ledbuiltin"; // TODO-53 replace with string no need to parameterize

unsigned long nextLoopTime = 0;
float value; // Time per blink (each phase)

void set(const float v) {
  if (value > v) { // May be waiting on long time, bring up
    nextLoopTime = millis();
  }
  value = v;
  #ifdef CONTROL_BLINKEN_DEBUG
    Serial.print(F("\nSetting Blink time (s) to:")); Serial.println(value);
  #endif // CONTROL_BLINKEN_DEBUG
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void inputReceived(const String &payload) {
#pragma GCC diagnostic pop
  float v = payload.toFloat(); // Copied to pin in the loop 
  set(v);
}

// TODO-25 temporary patch till new control.cpp ready
void dispatchLeaf(const String &topicLeaf, const String &payload) {
  if (topicLeaf == inputTopic) {
    inputReceived(payload);
  }
}

void setup() {
  set(CONTROL_BLINKEN_S); // default time            
  Mqtt->subscribe(inputTopic);
}

void loop() {
  if (nextLoopTime <= millis()) {
    value = !value;
    Mqtt->messageSend(outputTopic, !value, true, 1);
    nextLoopTime = millis() + value*1000;
  }
}

} //namespace cBlinken
#endif // CONTROL_BLINKEN_WANT
