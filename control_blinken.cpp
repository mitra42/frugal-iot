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
  error control_blinken does not have all requirements in _configuration.h: CONTROL_BLINKEN_S
#endif

#include <Arduino.h>
#include "control_blinken.h"
#include "system_discovery.h"
#include "system_mqtt.h"
#include "actuator_ledbuiltin.h"


namespace cBlinken {
String* inputTopic; 
const char* outputTopic = ACTUATOR_LEDBUILTIN_TOPIC; // TODO-53 replace with string no need to parameterize

unsigned long nextLoopTime = 0;
float value; // Time per blink (each phase)

void set(float v) {
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
void messageReceived(String &topic, String &payload) {
#pragma GCC diagnostic pop
  float v = payload.toFloat(); // Copied to pin in the loop 
  set(v);
}

void setup() {
  inputTopic = new String(*xDiscovery::topicPrefix + F("control_blinken_seconds"));
  set(CONTROL_BLINKEN_S); // default time            
  xMqtt::subscribe(*inputTopic, *messageReceived);
}

void loop() {
  if (nextLoopTime <= millis()) {
    xMqtt::messageSend(outputTopic, !value, true, 1); //TODO-53 XXX comment back in 
    nextLoopTime = millis() + value*1000;
  }
}

} //namespace cBlinken
#endif // CONTROL_BLINKEN_WANT
