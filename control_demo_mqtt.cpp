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

String *inTopic = new String(SENSOR_SHT85_TOPIC_HUMIDITY);
String *outTopic = new String(ACTUATOR_LEDBUILTIN_TOPIC);
bool value = false;

void messageReceived(String &topic, String &payload) {
  float humidity = payload.toFloat();
  #ifdef CONTROL_DEMO_MQTT_DEBUG
    Serial.print("cDemoMqtt received ");
    Serial.println(humidity);
  #endif
  bool newValue = humidity > 75;
  // Only send if its changed.
  if (newValue != value) {
    // TODO should not send this here (rule against sending with qos>0 withink messagReceived), it should be in a "loop" 
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
