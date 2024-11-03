/*
  Demo MQTT by listening for humidity and controlling LED

  Required SYSTEM_DISCOVERY_ORGANIZATION
  Optional CONTROL_DEMO_MQTT_DEBUG 
 */


#include "_settings.h"  // Settings for what to include etc

#ifdef CONTROL_DEMO_MQTT_WANT

#if (!defined(SYSTEM_DISCOVERY_ORGANIZATION))
  error actuator_ledbuiltin does not have all requirements in _configuration.h: SYSTEM_DISCOVERY_ORGANIZATION
#endif

#include <Arduino.h>
#include "_common.h"    // Main include file for Framework
#include "control_demo_mqtt.h"
#include "actuator_ledbuiltin.h"
#include "system_mqtt.h"
#include "system_discovery.h"
 
namespace cDemoMqtt {

String *inTopic;
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
    xMqtt::messageSend(*aLedbuiltin::topic, value, true, 1); // Note message will be queued , and sent outside of messageReceived handler
    value = newValue;
  }
}

void setup() {          
  inTopic = new String(xDiscovery::topicPrefix + SENSOR_SHT85_TOPIC_HUMIDITY);
   
  xMqtt::subscribe(*inTopic, *messageReceived);
  xMqtt::messageSend(*aLedbuiltin::topic, value, true, 1); // set initial value
}

} //namespace cDemoMqtt
#endif // CONTROL_DEMO_MQTT_WANT
