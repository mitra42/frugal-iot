/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  Based on the Blinken demo in the IDE

  Required CONTROL_BLINKEN_S // TODO-30 replace with a settable parameter
 */


#include "_settings.h"  // Settings for what to include etc

#ifdef CONTROL_BLINKEN_WANT

#if (!defined(CONTROL_BLINKEN_S))
  error control_blinken does not have all requirements in _configuration.h: CONTROL_BLINKEN_S
#endif

#include <Arduino.h>
#include "_common.h"    // Main include file for Framework
#include "control_blinken.h"
#include "system_discovery.h"
#include "system_mqtt.h"
#include "actuator_ledbuiltin.h"


namespace cBlinken {

unsigned long nextLoopTime = 0;
float value;
String *topic; 

void set(float v) {
  if (value > v) { // May be waiting on long time, bring up
    nextLoopTime = millis();
  }
  value = v;
  #ifdef CONTROL_BLINKEN_DEBUG
    Serial.print("\nSetting Blink time (s) to:"); Serial.println(value);
  #endif // CONTROL_BLINKEN_DEBUG
}

void messageReceived(String &topic, String &payload) {
  float v = payload.toFloat(); // Copied to pin in the loop 
  set(v);
}

void setup() {
  topic = new String(*xDiscovery::topicPrefix + "control_blinken_s");
  set(CONTROL_BLINKEN_S); // default time            
  xMqtt::subscribe(*topic, *messageReceived);
}

void loop() {
  if (nextLoopTime <= millis()) {
    xMqtt::messageSend(*aLedbuiltin::topic, !aLedbuiltin::value, true, 1);
    nextLoopTime = millis() + value*1000;
  }
}

} //namespace cBlinken
#endif // CONTROL_BLINKEN_WANT
