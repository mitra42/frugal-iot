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
#include "actuator_ledbuiltin.h" // TODO remove after sending back via MQTT
 
namespace cDemoMqtt {

String *topic = new String(SENSOR_SHT85_TOPIC_HUMIDITY);

void messageReceived(String &topic, String &payload) {
  float humidity = payload.toFloat();
  #ifdef CONTROL_DEMO_MQTT_DEBUG
    Serial.print("cDemoMqtt received ");
    Serial.println(humidity);
  #endif
  aLedbuiltin::value = humidity > 75;  // TODO move to sending this value via MQTT
}

void setup() {             
  xMqtt::subscribe(*topic, *messageReceived);
}

} //namespace cDemoMqtt
#endif // CONTROL_DEMO_MQTT_WANT
