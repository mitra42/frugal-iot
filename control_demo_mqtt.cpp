/*
  Demo MQTT by listening for humidity and controlling LED

  Optional CONTROL_DEMO_MQTT_DEBUG 
 */


#include "_settings.h"  // Settings for what to include etc

#ifdef CONTROL_DEMO_MQTT_WANT


#include <Arduino.h>
#include "control_demo_mqtt.h"
#include "actuator_ledbuiltin.h"  // TODO-43 remove dependency once controllable.
#include "system_mqtt.h"
#include "sensor_sht85.h" // TODO-43 remove dependency once controllable.
 
namespace cDemoMqtt {

String *inTopic;
bool value = false;

void send() {
    xMqtt::messageSend(aLedbuiltin::actuator_ledbuiltin.topic, value, true, 1); // Note message will be queued , and sent outside of messageReceived handler
}

void messageReceived(String &topic, String &payload) {
  float humidity = payload.toFloat();
  #ifdef CONTROL_DEMO_MQTT_DEBUG
    Serial.print(F("cDemoMqtt received ")); Serial.println(humidity);
  #endif
  bool newValue = humidity > CONTROL_DEMO_MQTT_HUMIDITY_MAX;
  // Only send if its changed.
  if (newValue != value) {
    value = newValue;
    send();
  }
}

void setup() {          
  inTopic = sSHT85::topicH;
   
  xMqtt::subscribe(*inTopic, *messageReceived);
  send();
}

} //namespace cDemoMqtt
#endif // CONTROL_DEMO_MQTT_WANT
