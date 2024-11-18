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

bool value = false;

void act() {
  static String* topic =  new String(*xDiscovery::topicPrefix + ACTUATOR_LEDBUILTIN_TOPIC); 
  xMqtt::messageSend(*topic, value, true, 1); // Note message will be queued , and sent outside of messageReceived handler
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void messageReceived(String &topic, String &payload) {
#pragma GCC diagnostic pop
  float humidity = payload.toFloat();
  #ifdef CONTROL_DEMO_MQTT_DEBUG
    Serial.print(F("cDemoMqtt received ")); Serial.println(humidity);
  #endif
  bool newValue = humidity > CONTROL_DEMO_MQTT_HUMIDITY_MAX;
  // Only send if its changed.
  if (newValue != value) {
    value = newValue;
    act();
  }
}

void setup() {          
  String *inTopic = new String(*xDiscovery::topicPrefix + F(SENSOR_SHT85_TOPIC_HUMIDITY));
  xMqtt::subscribe(*inTopic, *messageReceived);
  act();
}

} //namespace cDemoMqtt
#endif // CONTROL_DEMO_MQTT_WANT
