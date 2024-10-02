/*
  Demo of MQTT

  Required:
 */


#include "_settings.h"  // Settings for what to include etc
#include "actuator_ledbuiltin.h" // What we'll do

#ifdef CONTROL_DEMO_MQTT_WANT
#include <Arduino.h>
#include "_common.h"    // Main include file for Framework
#include "control_demo_mqtt.h"

namespace cDemoMqtt {

unsigned long nextLoopTime = 0;
String* sTopic = new String(SENSOR_SHT85_TOPIC_TEMPERATURE); 

void demoHandler(String &topic, String &payload) {
  #ifdef CONTROL_DEMO_MQTT_WANT
    Serial.println("handling: " + topic + " - " + payload);
  #endif // CONTROL_DEMO_MQTT_WANT
}

void setup() {  
  subscribe(*sTopic, &demoHandler);
}

/* TODO Remove I think
void loop() {
  if (nextLoopTime <= millis()) {
    aLedbuiltin::value = !aLedbuiltin::value;
    #ifdef CONTROL_BLINKEN_DEBUG
      Serial.print("Set LED to ");
      Serial.println(aLedbuiltin::value);
    #endif // CONTROL_BLINKEN_DEBUG
    nextLoopTime = millis() + CONTROL_BLINKEN_MS;
  }
}
*/
} //namespace cBlinken
#endif // CONTROL_DEMO_MQTT_WANT
