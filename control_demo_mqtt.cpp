/*
  Demo MQTT by listening for humidity and controlling LED

  Optional CONTROL_DEMO_MQTT_DEBUG 
 */


#include "_settings.h"  // Settings for what to include etc

#ifdef CONTROL_DEMO_MQTT_WANT
#include <Arduino.h>
#include "_common.h"    // Main include file for Framework
#include "control_demo_mqtt.h"
#include "system_mqtt.h"
 
namespace cDemoMqtt {

String *inTopic = new String(SYSTEM_DISCOVERY_PROJECT SYSTEM_DISCOVERY_DEVICE SENSOR_SHT85_TOPIC_HUMIDITY); // TODO-29 will come from config
String *outTopic = new String(SYSTEM_DISCOVERY_PROJECT SYSTEM_DISCOVERY_DEVICE ACTUATOR_LEDBUILTIN_TOPIC);  // TODO-29 will come from config
bool value = false;

void messageReceived(String &topic, String &payload) {
  float humidity = payload.toFloat();
  #ifdef CONTROL_DEMO_MQTT_DEBUG
    Serial.print("cDemoMqtt received ");
    Serial.println(humidity);
  #endif
  bool newValue = humidity > CONTROL_DEMO_MQTT_HUMIDITY_MAX;
  // Only send if its changed.
  if (newValue != value) {
    // TODO-33 should not send this here (rule against sending with qos>0 withink messagReceived), it should be in a "loop" 
    xMqtt::messageSend(*outTopic, value, true, 1);
    value = newValue;
  }
}

void setup() {             
  xMqtt::subscribe(*inTopic, *messageReceived);
  xMqtt::messageSend(*outTopic, value, true, 1); // set initial value
}

} //namespace cDemoMqtt
#endif // CONTROL_DEMO_MQTT_WANT
